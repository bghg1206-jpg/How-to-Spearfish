#!/usr/bin/env python3
"""
Shadowing lint for Unreal base-class members.

UE5 builds treat MSVC C4458 ("declaration of 'X' hides class member") as an error, and it fires for members
inherited from engine classes too - including private ones such as AActor::Owner. Clang's -Wshadow only
looks at the class's own fields, so this script fills the gap: it reads clang's AST dump of every module
source, resolves each project class's engine ancestry and reports parameters/locals named like a member of
one of those engine classes.

Usage: lint_shadow.py <clang++> <module_dir> <flags...>   (flags as used by run_module_check.sh)
"""
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

# Data members (any access level) of the engine classes our module derives from, per UE 5.5 headers.
ENGINE_MEMBERS = {
    'UObject': {'ClassPrivate', 'NamePrivate', 'OuterPrivate', 'ObjectFlags', 'InternalIndex'},
    'AActor': {'PrimaryActorTick', 'Owner', 'Instigator', 'Role', 'RemoteRole', 'Tags', 'Children', 'Layers', 'ParentComponent',
               'RootComponent', 'InputComponent', 'OwnedComponents', 'InstanceComponents', 'BlueprintCreatedComponents',
               'ReplicatedMovement', 'AttachmentReplication', 'NetDriverName', 'NetDormancy', 'NetUpdateFrequency',
               'MinNetUpdateFrequency', 'NetPriority', 'NetCullDistanceSquared', 'NetTag', 'InitialLifeSpan', 'CustomTimeDilation',
               'SpawnCollisionHandlingMethod', 'bHidden', 'bReplicates', 'bCanBeDamaged', 'bAlwaysRelevant', 'LastRenderTime',
               'InputPriority', 'TimerHandle_LifeSpanExpired', 'CreationTime', 'HiddenEditorViews', 'DetachFence'},
    'UActorComponent': {'PrimaryComponentTick', 'ComponentTags', 'AssetUserData', 'OwnerPrivate', 'WorldPrivate', 'bRegistered',
                        'bIsActive', 'bAutoActivate', 'bReplicates', 'UCSModifiedProperties', 'CreationMethod'},
    'USceneComponent': {'AttachParent', 'AttachSocketName', 'AttachChildren', 'ClientAttachedChildren', 'RelativeLocation',
                        'RelativeRotation', 'RelativeScale3D', 'ComponentVelocity', 'Bounds', 'Mobility', 'bVisible',
                        'bHiddenInGame', 'PhysicsVolume', 'ComponentToWorld', 'DetailMode'},
    'UPrimitiveComponent': {'BodyInstance', 'MinDrawDistance', 'LDMaxDrawDistance', 'CachedMaxDrawDistance', 'TranslucencySortPriority',
                            'CustomDepthStencilValue', 'MoveIgnoreActors', 'MoveIgnoreComponents', 'LightingChannels',
                            'OnComponentHit', 'OnComponentBeginOverlap', 'OnComponentEndOverlap', 'CastShadow', 'BoundsScale'},
    'UMeshComponent': {'OverrideMaterials', 'OverlayMaterial', 'OverlayMaterialMaxDrawDistance'},
    'UStaticMeshComponent': {'StaticMesh', 'ForcedLodModel', 'MinLOD', 'LODData', 'OverriddenLightMapRes', 'StreamingDistanceMultiplier'},
    'UInstancedStaticMeshComponent': {'PerInstanceSMData', 'PerInstanceSMCustomData', 'NumCustomDataFloats', 'InstancingRandomSeed',
                                      'InstanceStartCullDistance', 'InstanceEndCullDistance', 'InstanceReorderTable'},
    'UHierarchicalInstancedStaticMeshComponent': {'ClusterTreePtr', 'SortedInstances', 'NumBuiltInstances', 'BuiltInstanceBounds'},
    'UShapeComponent': {'ShapeColor', 'ShapeBodySetup', 'AreaClass'},
    'UBoxComponent': {'BoxExtent', 'LineThickness'},
    'USphereComponent': {'SphereRadius'},
    'UCapsuleComponent': {'CapsuleHalfHeight', 'CapsuleRadius'},
    'UTextRenderComponent': {'Text', 'TextMaterial', 'Font', 'HorizontalAlignment', 'VerticalAlignment', 'TextRenderColor', 'XScale',
                             'YScale', 'WorldSize', 'InvDefaultSize', 'HorizSpacingAdjust', 'VertSpacingAdjust'},
    'UCameraComponent': {'FieldOfView', 'OrthoWidth', 'AspectRatio', 'PostProcessSettings', 'PostProcessBlendWeight', 'ProjectionMode'},
    'ULightComponentBase': {'Intensity', 'LightColor', 'CastShadows', 'IndirectLightingIntensity', 'VolumetricScatteringIntensity'},
    'APawn': {'Controller', 'PlayerState', 'LastHitBy', 'ControlInputVector', 'LastControlInputVector', 'BaseEyeHeight',
              'AutoPossessPlayer', 'AutoPossessAI', 'AIControllerClass', 'PreviousController'},
    'ACharacter': {'Mesh', 'CharacterMovement', 'CapsuleComponent', 'ArrowComponent', 'BaseTranslationOffset', 'BaseRotationOffset',
                   'JumpKeyHoldTime', 'JumpCurrentCount', 'JumpMaxCount', 'JumpMaxHoldTime', 'bPressedJump', 'ReplicatedBasedMovement'},
    'UMovementComponent': {'Velocity', 'UpdatedComponent', 'UpdatedPrimitive', 'PlaneConstraintNormal', 'PlaneConstraintOrigin',
                           'MoveComponentFlags'},
    'UNavMovementComponent': {'NavAgentProps', 'MovementState', 'PathFollowingComp'},
    'UPawnMovementComponent': {'PawnOwner'},
    'UCharacterMovementComponent': {'CharacterOwner', 'Acceleration', 'MovementMode', 'CustomMovementMode', 'GravityScale',
                                    'MaxStepHeight', 'JumpZVelocity', 'MaxWalkSpeed', 'MaxFlySpeed', 'MaxSwimSpeed', 'MaxAcceleration',
                                    'CurrentFloor', 'LastUpdateLocation', 'LastUpdateRotation', 'LastUpdateVelocity',
                                    'PendingImpulseToApply', 'PendingForceToApply', 'PendingLaunchVelocity', 'RequestedVelocity',
                                    'Mass', 'Buoyancy', 'RotationRate', 'ClientPredictionData', 'ServerPredictionData',
                                    'NetworkSmoothingMode', 'AnalogInputModifier', 'CurrentRootMotion', 'GroundFriction',
                                    'BrakingFriction', 'AirControl'},
    'AController': {'Pawn', 'Character', 'PlayerState', 'ControlRotation', 'StartSpot', 'StateName', 'TransformComponent', 'OldPawn'},
    'APlayerController': {'Player', 'PlayerInput', 'PlayerCameraManager', 'MyHUD', 'AcknowledgedPawn', 'NetConnection', 'RotationInput',
                          'CheatManager', 'HiddenActors', 'InputYawScale', 'InputPitchScale', 'bShowMouseCursor', 'CurrentMouseCursor'},
    'APlayerState': {'Score', 'PlayerId', 'CompressedPing', 'PlayerNamePrivate', 'UniqueId', 'SavedNetworkAddress', 'StartTime',
                     'PawnPrivate', 'bIsSpectator', 'bOnlySpectator', 'bIsABot', 'bIsInactive'},
    'AGameModeBase': {'GameSession', 'GameState', 'GameStateClass', 'PlayerControllerClass', 'PlayerStateClass', 'HUDClass',
                      'DefaultPawnClass', 'SpectatorClass', 'GameSessionClass', 'OptionsString', 'DefaultPlayerName',
                      'bUseSeamlessTravel', 'bStartPlayersAsSpectators', 'bPauseable', 'Pausers'},
    'AGameStateBase': {'GameModeClass', 'AuthorityGameMode', 'SpectatorClass', 'PlayerArray', 'ReplicatedWorldTimeSecondsDouble',
                       'ServerWorldTimeSecondsDelta'},
    'AHUD': {'PlayerOwner', 'Canvas', 'DebugCanvas', 'bShowHUD', 'bShowDebugInfo', 'HitBoxMap', 'PostRenderedActors', 'DebugDisplay'},
    'UGameInstance': {'WorldContext', 'LocalPlayers', 'OnlineSession', 'ReferencedObjects', 'TimerManager', 'LatentActionManager',
                      'SubsystemCollection'},
    'USubsystem': {'InternalOwningSubsystem'},
    'UProceduralMeshComponent': {'ProcMeshSections', 'CollisionConvexElems', 'LocalBounds', 'ProcMeshBodySetup', 'AsyncBodySetupQueue'},
    'UDeveloperSettings': {'CategoryName', 'SectionName'},
}

RECORD = re.compile(r'(?:CXXRecordDecl|ClassTemplateSpecializationDecl) (0x[0-9a-f]+) .*?\b(?:class|struct) (\w+)\b(?: definition)?')
BASE = re.compile(r"^[| `]*-(?:public|protected|private)(?: virtual)? '([^']+)'")
METHOD = re.compile(r'(CXXMethodDecl|CXXConstructorDecl|CXXDestructorDecl) (0x[0-9a-f]+)(?: parent (0x[0-9a-f]+))?')
VAR = re.compile(r'(ParmVarDecl|VarDecl) 0x[0-9a-f]+ <([^>]*)> (?:col|line):?[0-9:]* (?:used |referenced |invalid )*(\w+) \'')
FILE_IN_LOC = re.compile(r'<(/[^:>]+):(\d+):\d+')
LINE_IN_LOC = re.compile(r'<line:(\d+):\d+')


def depth_of(line):
    match = re.match(r'^([| `]*)[-]', line)
    return len(match.group(1)) if match else 0


def analyse(cxx, module_dir, flags, source):
    proc = subprocess.run([cxx, *flags, '-Xclang', '-ast-dump', '-fno-color-diagnostics', source],
                          capture_output=True, text=True, errors='replace')
    lines = proc.stdout.splitlines()
    records = {}          # addr -> name
    bases = {}            # name -> [base names]
    record_stack = []     # (depth, name)
    method_stack = []     # (depth, class name)
    findings = []
    current_file = None
    current_line = 0
    pending_record = None
    for line in lines:
        depth = depth_of(line)
        file_match = FILE_IN_LOC.search(line)
        if file_match:
            current_file, current_line = file_match.group(1), int(file_match.group(2))
        else:
            line_match = LINE_IN_LOC.search(line)
            if line_match:
                current_line = int(line_match.group(1))
        while record_stack and depth <= record_stack[-1][0]:
            record_stack.pop()
        while method_stack and depth <= method_stack[-1][0]:
            method_stack.pop()

        base_match = BASE.match(line)
        if base_match and pending_record and depth == pending_record[0] + 2:
            base = re.sub(r'^(class|struct) ', '', base_match.group(1)).split('<')[0].split('::')[-1]
            bases.setdefault(pending_record[1], []).append(base)
            continue

        record_match = RECORD.search(line)
        if record_match and ('definition' in line):
            addr, name = record_match.groups()
            records[addr] = name
            record_stack.append((depth, name))
            pending_record = (depth, name)
            continue

        method_match = METHOD.search(line)
        if method_match:
            parent = method_match.group(3)
            owner = records.get(parent) if parent else (record_stack[-1][1] if record_stack else None)
            if owner:
                method_stack.append((depth, owner))
            continue

        var_match = VAR.search(line)
        if var_match and method_stack and current_file and current_file.startswith(module_dir):
            findings.append((method_stack[-1][1], var_match.group(3), current_file, current_line))
    return bases, findings


def ancestors(name, bases, seen=None):
    seen = seen if seen is not None else set()
    for base in bases.get(name, []):
        if base not in seen:
            seen.add(base)
            ancestors(base, bases, seen)
    return seen


def main():
    cxx, module_dir = sys.argv[1], os.path.abspath(sys.argv[2])
    flags = [f for f in sys.argv[3:] if f not in ('-Werror',)]
    sources = []
    for root, _, files in os.walk(module_dir):
        sources += [os.path.join(root, f) for f in files if f.endswith('.cpp')]
    all_bases, all_findings = {}, []
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        for bases, findings in pool.map(lambda s: analyse(cxx, module_dir, flags, s), sorted(sources)):
            for key, value in bases.items():
                all_bases.setdefault(key, value)
            all_findings += findings

    problems = set()
    for owner, var, path, line in all_findings:
        for ancestor in ancestors(owner, all_bases):
            if var in ENGINE_MEMBERS.get(ancestor, ()):
                problems.add(f'{os.path.relpath(path, module_dir)}:{line}: \'{var}\' in {owner} hides {ancestor}::{var} (C4458, an error in UE5 builds)')
    for problem in sorted(problems):
        print('SHADOW:', problem)
    print(f'lint_shadow: {len(sources)} sources, {len(problems)} problem(s)')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())

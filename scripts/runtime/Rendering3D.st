RenderingUploadedElement ![
initialize
    super initialize.
    lastUploadedFrame := -1.
    uploadedIndex := -1.
].

RenderingUploadedElement ![
lastUploadedFrame
    ^ lastUploadedFrame
].

RenderingUploadedElement ![
uploadedIndex
    ^ uploadedIndex
].

MeshMetallicRoughnessMaterial ![
initialize
    super initialize.
    baseColor := Color white.
    metallicFactor := 1.0.
    roughnessFactor := 1.0.
    occlusionFactor := 1.0.
    isUnlit := false
].

MeshMetallicRoughnessMaterial ![
baseColor
    ^ baseColor
].

MeshMetallicRoughnessMaterial ![
baseColor: aColor
    baseColor := aColor
].

MeshMetallicRoughnessMaterial ![
baseColorTexture
    ^ baseColorTexture
].

MeshMetallicRoughnessMaterial ![
baseColorTexture: aTexture
    baseColorTexture := aTexture
].

MeshMetallicRoughnessMaterial ![
metallicFactor
    ^ metallicFactor
].

MeshMetallicRoughnessMaterial ![
metallicFactor: aNumber
    metallicFactor := aNumber
].

MeshMetallicRoughnessMaterial ![
roughnessFactor
    ^ roughnessFactor
].

MeshMetallicRoughnessMaterial ![
roughnessFactor: aNumber
    roughnessFactor := aNumber
].

MeshMetallicRoughnessMaterial ![
metallicRoughnessTexture
    ^ metallicRoughnessTexture
].

MeshMetallicRoughnessMaterial ![
metallicRoughnessTexture: aTexture
    metallicRoughnessTexture := aTexture
].


MeshMetallicRoughnessMaterial ![
occlusionFactor
    ^ occlusionFactor
].

MeshMetallicRoughnessMaterial ![
occlusionFactor: aNumber
    occlusionFactor := aNumber
].

MeshMetallicRoughnessMaterial ![
occlusionTexture
    ^ occlusionTexture
].

MeshMetallicRoughnessMaterial ![
occlusionTexture: aTexture
    occlusionTexture := aTexture
].

MeshPrimitive ![
initialize
    super initialize.
    positions := #().
    normals := #().
    texcoords := #().
    tangents4 := #().
].


MeshPrimitive ![
material
    ^ material
].

MeshPrimitive ![
material: aMaterial
    material := aMaterial
].

MeshPrimitive ![
positions
    ^ positions
].

MeshPrimitive ![
positions: anArrayOfVector3
    positions := anArrayOfVector3
].

MeshPrimitive ![
normals
    ^ normals
].

MeshPrimitive ![
normals: anArrayOfVector3
    normals := anArrayOfVector3
].

MeshPrimitive ![
texcoords
    ^ texcoords
].

MeshPrimitive ![
texcoords: anArrayOfVector2
    texcoords := anArrayOfVector2
].

MeshPrimitive ![
tangents4
    ^ tangents4
].

MeshPrimitive ![
tangents4: anArrayOfVector4
    tangents4 := anArrayOfVector4
].

MeshPrimitive ![
indices
    ^ indices
].

MeshPrimitive ![
indices: anArrayOfInteger
    indices := anArrayOfInteger
].

Model3D ![
initialize
    super initialize.
    primitives := #().
].

Model3D ![
primitives
    ^ primitives
].

Model3D ![
primitives: anArrayOfPrimitives
    primitives := anArrayOfPrimitives
].

RenderObject3D ![
model
    ^ model
].

RenderObject3D ![
model: aModel
    model := aModel
].

RenderObject3D ![
modelMatrix
    ^ modelMatrix
].

RenderObject3D ![
modelMatrix: aMatrix
    modelMatrix := aMatrix
].
Object subclass: #MeshBuilder instanceVariables: #(currentMaterial positions normals texcoords tangents4 indices baseVertex primitives).

MeshBuilder ![
initialize
    super initialize.
    primitives := ArrayList new.
    self resetPrimitive.
].

MeshBuilder ![
resetPrimitive
    positions := ArrayList new.
    normals := ArrayList new.
    texcoords := ArrayList new.
    tangents4 := ArrayList new.
    indices := ArrayList new.
    baseVertex := 0.
].

MeshBuilder ![
currentMaterial
    ^ currentMaterial
].

MeshBuilder ![
currentMaterial: aMaterial
    currentMaterial := aMaterial
].

MeshBuilder ![
beginTriangles
    baseVertex := positions size.
].

MeshBuilder ![
addPx: x y: y z: z
    positions add: (Vector3 x: x y: y z: z).
].

MeshBuilder ![
addNx: x y: y z: z
    normals add: (Vector3 x: x y: y z: z).
].

MeshBuilder ![
addTriangleI0: i0 i1: i1 i2:i2
    indices add: (baseVertex + i0); add: (baseVertex + i1); add: (baseVertex + i2)
].

MeshBuilder ![
addCubeWithMinX: minX minY: minY minZ: minZ maxX: maxX maxY: maxY maxZ: maxZ
	self
		"Left"
		beginTriangles;
		addPx: minX y: minY z: minZ; addNx: -1.0 y: 0.0 z: 0.0;
		addPx: minX y: maxY z: minZ; addNx: -1.0 y: 0.0 z: 0.0;
		addPx: minX y: maxY z: maxZ; addNx: -1.0 y: 0.0 z: 0.0;
		addPx: minX y: minY z: maxZ; addNx: -1.0 y: 0.0 z: 0.0;
		addTriangleI0: 1 i1: 0 i2: 2;
		addTriangleI0: 3 i1: 2 i2: 0;

		"Right"
		beginTriangles;
		addPx: maxX y: minY z: minZ; addNx: 1.0 y: 0.0 z: 0.0;
		addPx: maxX y: maxY z: minZ; addNx: 1.0 y: 0.0 z: 0.0;
		addPx: maxX y: maxY z: maxZ; addNx: 1.0 y: 0.0 z: 0.0;
		addPx: maxX y: minY z: maxZ; addNx: 1.0 y: 0.0 z: 0.0;
		addTriangleI0: 0 i1: 1 i2: 2;
		addTriangleI0: 2 i1: 3 i2: 0;

		"Top"
		beginTriangles;
		addPx: minX y: maxY z: minZ; addNx: 0.0 y: 1.0 z: 0.0;
		addPx: maxX y: maxY z: minZ; addNx: 0.0 y: 1.0 z: 0.0;
		addPx: maxX y: maxY z: maxZ; addNx: 0.0 y: 1.0 z: 0.0;
		addPx: minX y: maxY z: maxZ; addNx: 0.0 y: 1.0 z: 0.0;
		addTriangleI0: 1 i1: 0 i2: 2;
		addTriangleI0: 3 i1: 2 i2: 0;

		"Bottom"
		beginTriangles;
		addPx: minX y: minY z: minZ; addNx: 0.0 y: -1.0 z: 0.0;
		addPx: maxX y: minY z: minZ; addNx: 0.0 y: -1.0 z: 0.0;
		addPx: maxX y: minY z: maxZ; addNx: 0.0 y: -1.0 z: 0.0;
		addPx: minX y: minY z: maxZ; addNx: 0.0 y: -1.0 z: 0.0;
		addTriangleI0: 0 i1: 1 i2: 2;
		addTriangleI0: 2 i1: 3 i2: 0;

		"Back"
		beginTriangles;
		addPx: minX y: minY z: minZ; addNx: 0.0 y: 0.0 z: -1.0;
		addPx: maxX y: minY z: minZ; addNx: 0.0 y: 0.0 z: -1.0;
		addPx: maxX y: maxY z: minZ; addNx: 0.0 y: 0.0 z: -1.0;
		addPx: minX y: maxY z: minZ; addNx: 0.0 y: 0.0 z: -1.0;
		addTriangleI0: 1 i1: 0 i2: 2;
		addTriangleI0: 3 i1: 2 i2: 0;

		"Front"
		beginTriangles;
		addPx: minX y: minY z: maxZ; addNx: 0.0 y: 0.0 z: 1.0;
		addPx: maxX y: minY z: maxZ; addNx: 0.0 y: 0.0 z: 1.0;
		addPx: maxX y: maxY z: maxZ; addNx: 0.0 y: 0.0 z: 1.0;
		addPx: minX y: maxY z: maxZ; addNx: 0.0 y: 0.0 z: 1.0;
		addTriangleI0: 0 i1: 1 i2: 2;
		addTriangleI0: 2 i1: 3 i2: 0
].

MeshBuilder ![
addCubeWithWidth: width height: height depth: depth
	self addCubeWithMinX: width * -0.5 minY: height * -0.5 minZ: depth * -0.5 maxX: width * 0.5 maxY: height * 0.5 maxZ: depth * 0.5
].

MeshBuilder ![
finishPrimitive
    | primitive |
    primitive := MeshPrimitive new
        material: currentMaterial;
        positions: positions asArray;
        normals: normals asArray;
        texcoords: texcoords asArray;
        tangents4: tangents4 asArray;
        indices: indices asArray;
        yourself.
    self resetPrimitive.
    primitives add: primitive.
    self resetPrimitive.
    ^ primitive
].

MeshBuilder ![
finishModel
    ^ Model3D new
        primitives: primitives asArray;
        yourself.
].
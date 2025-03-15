Object subclass: #ReversibleAffineTransform3D instanceVariables: #(matrix inverseMatrix translation inverseTranslation).

ReversibleAffineTransform3D class ![
identity
    ^ self basicNew setIdentity
].

ReversibleAffineTransform3D ![
setIdentity
    matrix := Matrix3x3 identity.
    inverseMatrix := Matrix3x3 identity.
    translation := Vector3 zeros.
    inverseTranslation := Vector3 zeros.
].

ReversibleAffineTransform3D class ![
scale: scale rotation: rotation translation: translation
    ^ self basicNew setScale: scale rotation: rotation translation: translation
].

ReversibleAffineTransform3D ![
setScale: scale rotation: rotation translation: aTranslation
    | orientationMatrix scaleMatrix inverseScaleMatrix |
    orientationMatrix := rotation asMatrix3x3.
    scaleMatrix := Matrix3x3 scale: scale.
    inverseScaleMatrix := Matrix3x3 scale: scale reciprocal.
    self setMatrix: orientationMatrix * scaleMatrix inverseMatrix: inverseScaleMatrix * orientationMatrix transpose translation: aTranslation
].

ReversibleAffineTransform3D ![
setMatrix: newMatrix inverseMatrix: newInverseMatrix translation: newTranslation
	matrix := newMatrix asMatrix3x3.
	inverseMatrix := newInverseMatrix asMatrix3x3.
	translation := newTranslation asVector3.
	inverseTranslation := inverseMatrix * translation negated
].

ReversibleAffineTransform3D ![
matrix
    ^ matrix
].

ReversibleAffineTransform3D ![
matrix: aMatrix3x3
    matrix := aMatrix3x3
].

ReversibleAffineTransform3D ![
inverseMatrix
    ^ inverseMatrix
].

ReversibleAffineTransform3D ![
inverseMatrix: aMatrix3x3
    inverseMatrix := aMatrix3x3
].

ReversibleAffineTransform3D ![
translation
    ^ translation
].

ReversibleAffineTransform3D ![
translation: aTranslation
    translation := aTranslation.
    inverseTranslation := inverseMatrix * translation negated
].

ReversibleAffineTransform3D ![
inverseTranslation
    ^ inverseTranslation
].

ReversibleAffineTransform3D ![
transformPosition: aPosition
    ^ (matrix * aPosition) + translation
].

ReversibleAffineTransform3D ![
transformVector: aVector
    ^ matrix * aVector
].

ReversibleAffineTransform3D ![
transformTransform: aTransform
    | affineTransform |
    affineTransform := aTransform asReversibleAffineTransform3D
    ^ self class basicNew
        matrix: matrix * affineTransform matrix;
        inverseMatrix: affineTransform inverseMatrix * inverseMatrix;
        translation: translation + (matrix * affineTransform translation);
        yourself
].

ReversibleAffineTransform3D ![
asReversibleAffineTransform3D
    ^ self
].

ReversibleAffineTransform3D ![
asMatrix4x
    ^ Matrix4x4 mat3: matrix translation: translation
].

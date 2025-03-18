Object subclass: #TRSTransform3D instanceVariables: #(scale rotation translation).

TRSTransform3D class ![
identity
    ^ self basicNew setIdentity
].

TRSTransform3D ![
initialize
    self setIdentity
].

TRSTransform3D ![
setIdentity
    scale := Vector3 ones.
    rotation := Quaternion identity.
    translation := Vector3 zeros.
].

TRSTransform3D ![
scale
    ^ scale
].

TRSTransform3D ![
scale: aScale
    scale := aScale
].

TRSTransform3D ![
rotation
    ^ rotation
].

TRSTransform3D ![
rotation: aRotation
    rotation := aRotation
].

TRSTransform3D ![
translation
    ^ translation
].

TRSTransform3D ![
translation: aTranslation
    translation := aTranslation
].

TRSTransform3D ![
transformPosition: aPosition
    ^ (rotation rotateVector: aPosition*scale) + translation
].

TRSTransform3D ![
asReversibleAffineTransform3D
    ^ ReversibleAffineTransform3D scale: scale rotation: rotation translation: translation
]
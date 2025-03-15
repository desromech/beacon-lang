Object subclass: #Ray3D instanceVariables: #(origin direction inverseDirection tmin tmax).
Object subclass: #Frustum instanceVariables: #(leftBottomNear rightBottomNear  leftTopNear 
                                                rightTopNear  leftBottomFar  rightBottomFar 
                                                leftTopFar  rightTopFar  planes).


Ray3D class ![
from: startPoint to: endPoint
	| vector vectorLength direction |
	vector := endPoint - startPoint.
	vectorLength := vector length.
	direction := vector / (vectorLength ~= 0 ifTrue: [vectorLength] ifFalse: [1]).
	^ self origin: startPoint direction: direction tmin: 0 tmax: vectorLength.
].

Ray3D class ![
origin: origin direction: direction tmin: tmin tmax: tmax
	^ self basicNew
		origin: origin;
		direction: direction;
		inverseDirection: direction reciprocal;
		tmin: tmin;
		tmax: tmax;
		yourself
].

Ray3D ![
printOn: aStream
    aStream nextPutAll: 'Ray3D origin: '; print: origin; nextPutAll: ' direction: '; print: direction
].

Ray3D ![
origin
    ^ origin
].

Ray3D ![
origin: aVector3
    origin := aVector3
].

Ray3D ![
direction
    ^ direction
].

Ray3D ![
direction: aVector3
    direction := aVector3
].

Ray3D ![
inverseDirection
    ^ inverseDirection
].

Ray3D ![
inverseDirection: aVector3
    inverseDirection := aVector3
].

Ray3D ![
tmin
    ^ tmin
].

Ray3D ![
tmin: aFloat
    tmin := tmin
].

Ray3D ![
tmax
    ^ tmax
].

Ray3D ![
tmax: aFloat
    tmax := tmax
].

Frustum ![
leftBottomNear
    ^ leftBottomNear
].

Frustum ![
leftBottomNear: aPoint
    leftBottomNear := aPoint
].

Frustum ![
rightBottomNear
    ^ rightBottomNear
].

Frustum ![
rightBottomNear: aPoint
    rightBottomNear := aPoint
].

Frustum ![
leftTopNear
    ^ leftTopNear
].

Frustum ![
leftTopNear: aPoint
    leftTopNear := aPoint
].

Frustum ![
rightTopNear
    ^ rightTopNear
].

Frustum ![
rightTopNear: aPoint
    rightTopNear := aPoint
].

Frustum ![
leftBottomFar
    ^ leftBottomFar
].

Frustum ![
leftBottomFar: aPoint
    leftBottomFar := aPoint
].
Frustum ![
rightBottomFar
    ^ rightBottomFar
].

Frustum ![
rightBottomFar: aPoint
    rightBottomFar := aPoint
].

Frustum ![
leftTopFar
    ^ leftTopFar
].

Frustum ![
leftTopFar: aPoint
    leftTopFar := aPoint
].

Frustum ![
rightTopFar
    ^ rightTopFar
].

Frustum ![
rightTopFar: aPoint
    rightTopFar := aPoint
].

Frustum ![
makeFrustumLeft: left right: right bottom: bottom top: top near: near far: far
	|  factor |
	leftBottomNear := Vector3 x: left y: bottom z: near negated.
	rightBottomNear := Vector3 x: right y: bottom z: near negated.
	leftTopNear := Vector3 x: left y: top z: near negated.
	rightTopNear := Vector3 x: right y: top z: near negated.

	factor := far / near.
	leftBottomFar := leftBottomNear * factor.
	rightBottomFar := rightBottomNear * factor.
	leftTopFar := leftTopNear * factor.
	rightTopFar := rightTopNear * factor.

	self computePlanes.
].

Frustum ![
makeOrthoLeft: left right: right bottom: bottom top: top near: near far: far
	leftBottomNear := Vector3 x: left y: bottom z: near negated.
	rightBottomNear := Vector3 x: right y: bottom z: near negated.
	leftTopNear := Vector3 x: left y: top z: near negated.
	rightTopNear := Vector3 x: right y: top z: near negated.

	leftBottomFar := Vector3 x: left y: bottom z: far negated.
	rightBottomFar := Vector3 x: right y: bottom z: far negated.
	leftTopFar := Vector3 x: left y: top z: far negated.
	rightTopFar := Vector3 x: right y: top z: far negated.

	self computePlanes.
].

Frustum ![
makePerspectiveFovY: fovy aspect: aspect near: near far: far
	| fovyRad top right |
	fovyRad := fovy degreesToRadians * 0.5.
	top := near * fovyRad tan.
	right := top * aspect.
	self makeFrustumLeft: right negated right: right bottom: top negated top: top near: near far: far
].

Frustum ![
computePlanes
].

Frustum ![
transformedWith: aTransform
    | result |

    result := Frustum basicNew.
    result leftBottomNear:  (aTransform transformPosition: leftBottomNear).
	result rightBottomNear: (aTransform transformPosition: rightBottomNear).
	result leftTopNear:     (aTransform transformPosition: leftTopNear).
	result rightTopNear:    (aTransform transformPosition: rightTopNear).

    result leftBottomFar:  (aTransform transformPosition: leftBottomFar).
	result rightBottomFar: (aTransform transformPosition: rightBottomFar).
	result leftTopFar:     (aTransform transformPosition: leftTopFar).
	result rightTopFar:    (aTransform transformPosition: rightTopFar).
    ^ result
].

Frustum ![
normalizedPointInNearPlane: normalizedPoint
	^ (leftBottomNear interpolateTo: rightBottomNear at: normalizedPoint x) interpolateTo:
		(leftTopNear interpolateTo: rightTopNear at: normalizedPoint x) at: normalizedPoint y.
].

Frustum ![
normalizedPointInFarPlane: normalizedPoint
	^ (leftBottomFar interpolateTo: rightBottomFar at: normalizedPoint x) interpolateTo:
		(leftTopFar interpolateTo: rightTopFar at: normalizedPoint x) at: normalizedPoint y.
].

Frustum ![
rayForNormalizedPoint: normalizedPoint
	| nearPoint farPoint |
	nearPoint := self normalizedPointInNearPlane: normalizedPoint.
	farPoint := self normalizedPointInFarPlane: normalizedPoint.
	^ Ray3D from: nearPoint to: farPoint
].

SceneCamera ![
viewFrustumWithAspectRatio: aspectRatio
    | newFrustum |
    newFrustum := Frustum new.
    isPerspective ifTrue: [
        newFrustum makePerspectiveFovY: fovY aspect: aspectRatio near: nearDistance far: farDistance
    ] ifFalse: [
        
    ].
    ^ newFrustum
].

SceneCamera ![
worldFrustumWithAspectRatio: aspectRatio
    | viewFrustum cameraTransform |
    viewFrustum := self viewFrustumWithAspectRatio: aspectRatio.
    cameraTransform := ReversibleAffineTransform3D scale: Vector3 ones rotation: orientation translation: location.
    ^ viewFrustum transformedWith: cameraTransform
].
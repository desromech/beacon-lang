Vector2 class ![
zeros
    ^ self x: 0 y: 0
].

Vector2 class ![
ones
    ^ self x: 1 y: 1
].

Vector2 ![
printOn: aStream
    aStream
        nextPutAll: '(Vector2 x: '; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPut: $)
].

Vector3 class ![
zeros
    ^ self x: 0 y: 0 z: 0
].

Vector3 class ![
ones
    ^ self x: 1 y: 1 z: 1
].

Vector3 ![
printOn: aStream
    aStream
        nextPutAll: '(Vector3 x: '; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPutAll: ' z: '; print: self z;
        nextPut: $)
].

Vector4 class ![
zeros
    ^ self x: 0 y: 0 z: 0 w: 0
].

Vector4 class ![
ones
    ^ self x: 1 y: 1 z: 1 w: 1
].

Vector4 ![
printOn: aStream
    aStream
        nextPutAll: '(Vector4 x: '; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPutAll: ' z: '; print: self z;
        nextPutAll: ' w: '; print: self w;
        nextPut: $)
].

Complex ![
printOn: aStream
    aStream
        nextPutAll: '(Complex r: '; print: self r;
        nextPutAll: ' i: '; print: self i;
        nextPut: $)
].

Quaternion class ![
identity
    ^ self x: 0 y: 0 z: 0 w: 1
].

Quaternion ![
printOn: aStream
    aStream
        nextPutAll: '(Quaternion x: '; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPutAll: ' z: '; print: self z;
        nextPutAll: ' w: '; print: self w;
        nextPut: $)
].


Matrix2x2 ![
printOn: aStream
    aStream
        nextPutAll: '(Matrix2x2 rows: {'; lf;
        print: self firstRow; nextPut: $.; lf;
        print: self secondRow; nextPut: $.; lf;
        nextPutAll: '})'
].

Matrix3x3 ![
printOn: aStream
    aStream
        nextPutAll: '(Matrix3x3 rows: {'; lf;
        print: self firstRow; nextPut: $.; lf;
        print: self secondRow; nextPut: $.; lf;
        print: self thirdRow; lf;
        nextPutAll: '})'
].

Matrix4x4 ![
printOn: aStream
    aStream
        nextPutAll: '(Matrix4x4 rows: {'; lf;
        print: self firstRow; nextPut: $.; lf;
        print: self secondRow; nextPut: $.; lf;
        print: self thirdRow; nextPut: $.; lf;
        print: self fourthRow; lf;
        nextPutAll: '})'
].

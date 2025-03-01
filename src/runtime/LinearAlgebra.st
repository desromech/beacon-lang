Vector2 ![
printOn: aStream
    aStream
        nextPutAll: '(Vector2 x:'; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPut: $)
].

Vector3 ![
printOn: aStream
    aStream
        nextPutAll: '(Vector3 x:'; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPutAll: ' z: '; print: self z;
        nextPut: $)
].

Vector4 ![
printOn: aStream
    aStream
        nextPutAll: '(Vector3 x:'; print: self x;
        nextPutAll: ' y: '; print: self y;
        nextPutAll: ' z: '; print: self z;
        nextPutAll: ' w: '; print: self w;
        nextPut: $)
].

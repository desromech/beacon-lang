AbstractBinaryFileStream ![
    lf
        self nextPut: 10
].

AbstractBinaryFileStream ![
    cr
        self nextPut: 13
].

AbstractBinaryFileStream ![
    crlf
        self cr; lf
].
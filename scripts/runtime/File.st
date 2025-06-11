File class ![
open: filename writeable: writeable
    ^ self open: filename writeable: writeable truncated: false
].

File class ![
openForReadFileNamed: aFileName
    ^ self open: aFileName writeable: false
].

File class ![
openForWriteFileNamed: aFileName
    ^ self open: aFileName writeable: true
].

File class ![
openForNewFileNamed: aFileName
    ^ self open: aFileName writeable: true truncated: true
].

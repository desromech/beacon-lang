DirectoryEntry ![
printOn: aStream
    isDirectory ifTrue: [aStream nextPutAll: '[dir]'].
    aStream nextPutAll: name
].

Directory ![
entries
    | entries nextEntry | 
    entries := ArrayList new.
    [(nextEntry := self nextEntry) ~~ nil] whileTrue: [
        entries add: nextEntry
    ].
    ^ entries asArray
].

Directory class ![
entriesOfPath: aPath
    | directory entries |
    directory := self open: aPath.
    directory == nil ifTrue: [ ^ nil ].

    entries := [
        directory entries
    ] ensure: [directory close].

    ^ entries
]

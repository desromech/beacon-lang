ArrayList ![
size
    ^ size
].

ArrayList ![
at: index
    ^ array basicAt: index
].

ArrayList ![
at: index put: element
    array basicAt: index put: element
].

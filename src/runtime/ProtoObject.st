
Message ![
selector
    ^ selector
].

ProtoObject ![
doesNotUnderstand: aMessage
    Stdio stdout nextPutAll: 'doesNotUnderstand:'; lf.
    Stdio stdout nextPutAll: self printString; lf.
    Stdio stdout nextPutAll: aMessage selector; lf.
].
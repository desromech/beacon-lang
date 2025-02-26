
Message ![
selector
    ^ selector
].

Message ![
selector: aSelector
    selector := aSelector
].

Message ![
arguments
    ^ arguments
].

Message ![
arguments: anArray
    arguments := anArray
].

MessageNotUnderstood ![
message
    ^ message
].

MessageNotUnderstood ![
message: aMessage
    message := aMessage
].

MessageNotUnderstood ![
receiver
    ^ receiver
].

MessageNotUnderstood ![
receiver: aReceiver
    receiver := aReceiver
].

MessageNotUnderstood ![
displayException
    Stdio stdout nextPutAll: 'doesNotUnderstand:'; lf.
    Stdio stdout nextPutAll: self printString; lf.
    Stdio stdout nextPutAll: message selector; lf.
].

ProtoObject ![
doesNotUnderstand: aMessage
    MessageNotUnderstood new
        message: aMessage;
        receiver: self;
        signal
].

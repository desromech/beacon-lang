Class ![
    name
        ^  name
].

Class ![
    name: aNameSymbol
        name := aNameSymbol
].

Metaclass ![
    thisClass
        ^ thisClass
].

Metaclass ![
    thisClass: aClass
        thisClass := aClass
].

Class ![
    subclass: subclassName instanceVariables: instanceVariables metaInstanceVariables: metaInstanceVariables
        | metaclass subclass |

    metaclass := Metaclass new.
    metaclass superclass: self class.
    metaclass instSize: self class instSize.
    metaclass objectKind: self class objectKind.
    metaclass setSlotsWithInstanceVariableNames: metaInstanceVariables.

    subclass := metaclass new.
    metaclass thisClass: subclass.
    subclass superclass: self.
    subclass instSize: self instSize.
    subclass objectKind: self objectKind.
    subclass name: subclassName.
    subclass setSlotsWithInstanceVariableNames: instanceVariables.

    SystemDictionary at: subclassName put: subclass.

    ^ subclass
].

Class ![
    subclass: subclassName instanceVariables: instanceVariables
    self subclass: subclassName instanceVariables: instanceVariables metaInstanceVariables: #()
].

Class ![
    subclass: subclassName.
    ^ self subclass: subclassName instanceVariables: #()
].
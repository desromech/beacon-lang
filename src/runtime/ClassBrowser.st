BorderedMorph subclass: #ClassBrowser instanceVariables: #(classTableMorph methodTableMorph codeTextArea currentClass).

ClassBrowser class ![
defaultColor
    ^ Color r: 0.8 g: 0.8 b: 0.8 a: 1.0
].

ClassBrowser class ![
defaultExtent
    ^ 640@480.
].

ClassBrowser ![
initialize
    super initialize.

    classTableMorph := TableMorph new.
    classTableMorph dataSource: (SimpleTableDataSource elements: SystemDictionary keys).

    classTableMorph onSelectionChangedDo: [:table | self onClassTableSelectionChanged: table selectedElements ].

    methodTableMorph := TableMorph new.
    methodTableMorph onSelectionChangedDo: [:table | self onMethodTableSelectionChanged: table selectedElements ].

    codeTextArea := CodeTextAreaMorph new.
    currentClass := nil.
    self addMorph: classTableMorph.
    self addMorph: methodTableMorph.
    self addMorph: codeTextArea.

    self layout: (VerticalMorphPackingLayout new
        addLayout: (HorizontalMorphPackingLayout new
            addMorph: classTableMorph proportion: 1;
            addMorph: methodTableMorph proportion: 1;
            yourself
            ) proportion: 1;
        addMorph: codeTextArea proportion: 1;
        yourself).
    self updateLayout
].

ClassBrowser ![
onClassTableSelectionChanged: selectedElements
    | clazz selectors |
    selectedElements size = 1 ifFalse: [ ^ self ].

    clazz := SystemDictionary atOrNil: selectedElements first.
    clazz isClass ifFalse: [^ self ].

    selectors := #().
    clazz methodDict ~~ nil ifTrue: [
        selectors := clazz methodDict keys.
    ].
    
    methodTableMorph dataSource: (SimpleTableDataSource elements: selectors).
    currentClass := clazz.
    self changed
].

ClassBrowser ![
onMethodTableSelectionChanged: selectedElements
    | method selector methodSourcePosition sourceText |
    selectedElements size = 1 ifFalse: [ ^ self ].
    selector := selectedElements first.

    method := currentClass methodDict atOrNil: selector.
    method == nil ifTrue: [ ^ self ].

    methodSourcePosition := method sourcePosition.
    methodSourcePosition == nil ifTrue: [
        sourceText := 'Source code is not available for ', selector printString
    ] ifFalse: [
        sourceText := methodSourcePosition sourceText.
    ].

    codeTextArea contents: sourceText
].


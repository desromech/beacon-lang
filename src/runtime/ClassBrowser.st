BorderedMorph subclass: #ClassBrowser instanceVariables: #(classTableMorph methodTableMorph codeTextArea).

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

    methodTableMorph := TableMorph new.
    codeTextArea := CodeTextAreaMorph new.
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


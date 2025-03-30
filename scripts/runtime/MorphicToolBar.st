BorderedMorph subclass: #ToolBarMorph instanceVariables: #(elements isMenuOpen).

ToolBarMorph class ![
defaultColor
    ^ Color lightGray.
].

ToolBarMorph class ![
defaultExtent
    ^ 100@30.
].

ToolBarMorph ![
initialize
    super initialize.
    elements := ArrayList new.
    isMenuOpen := false.

    self layout: HorizontalMorphPackingLayout new.
].

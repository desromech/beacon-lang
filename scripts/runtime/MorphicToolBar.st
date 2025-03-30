BorderedMorph subclass: #ToolBarMorph instanceVariables: #(isMenuOpen).

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

    self layout: HorizontalMorphPackingLayout new.
].


ToolBarMorph ![
addButtonWithLabel: label andAction: aBlock
    | button |
    button := SimpleButtonMorph new label: label; onClickAction: aBlock.
    self addMorph: button.
    self layout addMorph: button proportion: 0 expanded: true.
    self updateLayout
].

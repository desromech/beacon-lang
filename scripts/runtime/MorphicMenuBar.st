BorderedMorph subclass: #MenuBarMorph instanceVariables: #(layout elements).
Morph subclass: #MenuItemButton instanceVariables: #(label submenu).
BorderedMorph subclass: #MenuMorph instanceVariables: #(elements).

MenuBarMorph class ![
defaultColor
    ^ Color lightGray.
].

MenuBarMorph class ![
defaultExtent
    ^ 100@30.
].

MenuItemButton class ![
defaultColor
    ^ Color lightGray.
].

MenuItemButton class ![
defaultExtent
    ^ 50@30.
].

MenuBarMorph ![
initialize
    super initialize.
    elements := ArrayList new.

    self layout: HorizontalMorphPackingLayout new.
].

MenuBarMorph ![
addItem: label submenu: submenu
    | button |
    button := MenuItemButton new label: label; submenu: submenu; yourself.
    elements add: button.
    self addMorph: button.

    self layout addMorph: button proportion: 0 expanded: true.
    self updateLayout
].

MenuItemButton ![
initialize
    super initialize.
].

MenuItemButton ![
renderWithBuilder: renderingBuilder
    self hasMouseFocus ifTrue: [
        renderingBuilder solidRectangle: self localBounds color: Color lightGray
    ].
    renderingBuilder text: label rectangle: self localBounds color: Color black.
].

MenuItemButton ![
handleMorphMouseMotionEvent: anEvent
    Stdio stdout print: anEvent; cr.
    self grabMouseFocus.
    anEvent wasHandled: true.
    self changed
].

MenuItemButton ![
handleMorphMouseButtonDownEvent: anEvent
    self changed.
].

MorphEventVisitor ![
handleMorphMouseLeaveEvent: anEvent
    self changed
].

MenuItemButton ![
label
    ^ label
].

MenuItemButton ![
label: aString
    label := aString.
    self extent: (Font defaultFontFace measureTextExtent: label) + (10 @ 10)
].

MenuItemButton ![
submenu
    ^ submenu
].

MenuItemButton ![
submenu: aMenu
    submenu := aMenu
].

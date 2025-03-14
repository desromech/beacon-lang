BorderedMorph subclass: #MenuBarMorph instanceVariables: #(elements isMenuOpen).
Morph subclass: #MenuItemButtonMorph instanceVariables: #(label submenu actionBlock).
BorderedMorph subclass: #MenuMorph instanceVariables: #(elements ownerMenu).

MenuBarMorph class ![
defaultColor
    ^ Color lightGray.
].

MenuBarMorph class ![
defaultExtent
    ^ 100@30.
].

MenuMorph class ![
defaultColor
    ^ Color lightGray.
].

MenuItemButtonMorph class ![
defaultColor
    ^ Color lightGray.
].

MenuItemButtonMorph class ![
defaultExtent
    ^ 50@30.
].

MenuBarMorph ![
isMenuBar
    ^ true
].

MenuMorph ![
isMenuBar
    ^ false
].

MenuBarMorph ![
initialize
    super initialize.
    elements := ArrayList new.
    isMenuOpen := false.

    self layout: HorizontalMorphPackingLayout new.
].

MenuBarMorph ![
isMenuOpen
    ^ isMenuOpen
].

MenuBarMorph ![
isMenuOpen: aBoolean
    isMenuOpen := aBoolean
].

MenuBarMorph ![
toggleSubmenuOpen: aSubmenu atPosition: globalPosition
    isMenuOpen ifTrue: [
        isMenuOpen := false.
        self rootMorph killPopUps
    ] ifFalse: [
        isMenuOpen := true.
        self rootMorph popUpUnique: aSubmenu atPosition: globalPosition
    ]
].

MenuBarMorph ![
addItem: label submenu: submenu
    | button |
    button := MenuItemButtonMorph new label: label; submenu: submenu; yourself.
    submenu ownerMenu: self.
    elements add: button.
    self addMorph: button.

    self layout addMorph: button proportion: 0 expanded: true.
    self updateLayout
].

MenuMorph ![
initialize
    super initialize.
    elements := ArrayList new.

    self layout: VerticalMorphPackingLayout new.
].

MenuMorph ![
ownerMenu
    ^ ownerMenu
].

MenuMorph ![
ownerMenu: aMenuBarOrMenu
    ownerMenu := aMenuBarOrMenu
].

MenuMorph ![
addItem: label withAction: aBlock
    | button |
    button := MenuItemButtonMorph new label: label; actionBlock: aBlock; yourself.
    elements add: button.
    self addMorph: button.

    self layout addMorph: button proportion: 0 expanded: true.
    self extent: 200 @ (elements size * 30).
    self updateLayout
].

MenuMorph ![
addSeparator
].

MenuItemButtonMorph ![
initialize
    super initialize.
].

MenuItemButtonMorph ![
renderWithBuilder: renderingBuilder
    self hasMouseFocus ifTrue: [
        self owner isMenuBar ifTrue: [
            renderingBuilder solidRectangle: self localBounds color: Color lightGray
        ] ifFalse: [
            renderingBuilder solidRectangle: self localBounds color: Color lightBlue
        ]
    ].
    renderingBuilder text: label rectangle: self localBounds color: Color black.
].

MenuItemButtonMorph ![
handleMorphMouseMotionEvent: anEvent
    self grabMouseFocus.
    anEvent wasHandled: true.
    (self owner isMenuBar and: [self owner isMenuOpen]) ifTrue:[
        self rootMorph popUpUnique: submenu atPosition: (self transformLocalPositionToGlobal: (0.0 @ self extent y))
    ].

    self changed
].

MenuItemButtonMorph ![
handleMorphMouseButtonDownEvent: anEvent
    self owner isMenuBar ifTrue: [
        self owner toggleSubmenuOpen: submenu atPosition: (self transformLocalPositionToGlobal: (0.0 @ self extent y)).
    ] ifFalse: [
        "Stdio stdout print: self owner ownerMenu; lf."
        self owner ownerMenu isMenuOpen: false.
        self rootMorph killPopUps
    ].

    actionBlock value.
    self changed.
].

MorphEventVisitor ![
handleMorphMouseLeaveEvent: anEvent
    self changed
].

MenuItemButtonMorph ![
label
    ^ label
].

MenuItemButtonMorph ![
label: aString
    label := aString.
    self extent: (Font defaultFontFace measureTextExtent: label) + (10 @ 10)
].

MenuItemButtonMorph ![
submenu
    ^ submenu
].

MenuItemButtonMorph ![
submenu: aMenu
    submenu := aMenu
].

MenuItemButtonMorph ![
actionBlock
    ^ actionBlock
].

MenuItemButtonMorph ![
actionBlock: aBlockClosure
    actionBlock := aBlockClosure
].
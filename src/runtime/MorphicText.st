Object subclass: #TextEditorCursor instanceVariables: #().
Object subclass: #TextEditorBuffer instanceVariables: #(text).

BorderedMorph subclass: #TextAreaMorph instanceVariables: #(textBuffer).

TextAreaMorph class ![
defaultColor
    ^ Color white.
].

TextAreaMorph class ![
defaultExtent
    ^ 400@300.
].

TextAreaMorph ![
initialize
    super initialize.
    textBuffer := TextEditorBuffer new
].

TextAreaMorph ![
handleMorphMouseButtonDownEvent: anEvent
    Stdio stdout nextPutAll: 'grabKeyboardFocus'; lf.
    self grabKeyboardFocus.
    anEvent wasHandled: true
].

TextAreaMorph ![
handleMorphKeyboardDownEvent: anEvent
    Stdio stdout nextPutAll: 'handleMorphKeyboardDownEvent'; lf.
    anEvent wasHandled: true
].

TextAreaMorph ![
handleMorphKeyboardUpEvent: anEvent
    Stdio stdout nextPutAll: 'handleMorphKeyboardUpEvent'; lf.
    anEvent wasHandled: true
].

TextAreaMorph ![
handleMorphTextInputEvent: anEvent
    Stdio stdout nextPutAll: 'handleMorphTextInputEvent '; nextPutAll: anEvent text; lf.
    anEvent wasHandled: true
].
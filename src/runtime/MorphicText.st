Object subclass: #TextEditorCursor instanceVariables: #(line column).

TextEditorCursor ![
initialize
    super initialize.
    line := 1.
    column := 1.
].

TextEditorCursor ![
line
    ^ line
].

TextEditorCursor ![
line: anInteger
    line := anInteger
].

TextEditorCursor ![
column
    ^ column
].

TextEditorCursor ![
column: anInteger
    column := anInteger
].

Object subclass: #TextEditorBuffer instanceVariables: #(lines).

TextEditorBuffer ![
initialize
    super initialize.
    lines := ArrayList new.
    lines add: ''
].

TextEditorBuffer ![
lines
    ^ lines
].

TextEditorBuffer ![
insertText: text atCursorPosition: cursor
   | line leftText rightText insertedLine |
    line := lines at: cursor line.
    leftText := line first: cursor column.
    rightText := line allButFirst: cursor column.

    insertedLine := leftText , text, rightText.
    Stdio stdout nextPutAll: insertedLine; lf.
    cursor column: cursor column + 1.
    lines at: cursor line put: insertedLine
].

BorderedMorph subclass: #TextAreaMorph instanceVariables: #(textBuffer textColor cursor).

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
    textBuffer := TextEditorBuffer new.
    textColor := Color black.
    cursor := TextEditorCursor new.
].

TextAreaMorph ![
renderWithBuilder: renderingBuilder
    | fontFace lines lineHeight lineWidth linePositionY |
    fontFace := Font defaultFontFace.
    lineHeight := fontFace height.

    renderingBuilder solidRectangle: self bounds color: self color.
    
    lines := textBuffer lines.
    linePositionY := 0.

    1 to: lines size do: [:lineIndex |
        | line lineExtent lineRectangle |
        line := lines at: lineIndex.
        lineExtent := fontFace measureTextExtent: line.
        lineRectangle := 0@linePositionY extent: lineExtent.

        renderingBuilder text: line rectangle: lineRectangle color: textColor.

        linePositionY := linePositionY + lineHeight.
    ].
].

TextAreaMorph ![
handleMorphMouseButtonDownEvent: anEvent
    "Stdio stdout nextPutAll: 'grabKeyboardFocus'; lf."
    self grabKeyboardFocus.
    anEvent wasHandled: true
].

TextAreaMorph ![
handleMorphKeyboardDownEvent: anEvent
    "Stdio stdout nextPutAll: 'handleMorphKeyboardDownEvent'; lf."
    anEvent wasHandled: true
].

TextAreaMorph ![
handleMorphKeyboardUpEvent: anEvent
    "Stdio stdout nextPutAll: 'handleMorphKeyboardUpEvent'; lf."
    anEvent wasHandled: true
].

TextAreaMorph ![
handleMorphTextInputEvent: anEvent
    "Stdio stdout nextPutAll: 'handleMorphTextInputEvent '; nextPutAll: anEvent text; lf.
    Stdio stdout nextPutAll: 'handleMorphTextInputEvent '; nextPutAll: anEvent text class printString; lf."
    textBuffer insertText: anEvent text atCursorPosition: cursor.
    anEvent wasHandled: true.
    self changed.
].
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
   | line leftText rightText cursorColumn insertedLine |
    line := lines at: cursor line.

    cursorColumn := cursor column.
    leftText := line first: cursorColumn.
    rightText := line allButFirst: cursorColumn.
    insertedLine := leftText , text , rightText.

    cursor column: cursor column + 1.
    lines at: cursor line put: insertedLine
].

BorderedMorph subclass: #TextAreaMorph instanceVariables: #(textBuffer textColor cursor cursorColor).

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
    cursorColor := Color blue.
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
        | line lineExtent lineRectangle cursorPositionX cursorBounds |
        line := lines at: lineIndex.
        lineExtent := fontFace measureTextExtent: line.
        lineRectangle := 0@linePositionY extent: lineExtent.

        renderingBuilder text: line rectangle: lineRectangle color: textColor.

        lineIndex = cursor line ifTrue: [
            cursorPositionX := (fontFace measureTextExtent: line until: cursor column) x.
            cursorBounds := cursorPositionX @ linePositionY extent: 2@lineHeight.
            renderingBuilder solidRectangle: cursorBounds color: cursorColor.
        ].

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
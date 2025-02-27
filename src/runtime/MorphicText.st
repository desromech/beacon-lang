Object subclass: #TextEditorCursor instanceVariables: #(textBuffer line column).

TextEditorCursor ![
initialize
    super initialize.
    line := 1.
    column := 1.
].

TextEditorCursor ![
textBuffer
    ^ textBuffer
].

TextEditorCursor ![
textBuffer: aTextBuffer
    textBuffer := aTextBuffer
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

TextEditorCursor ![
moveLeft
    column > 1 ifTrue: [
        column := column - 1.
    ] ifFalse: [
        line > 1 ifTrue: [
            line := line - 1.
            column := (textBuffer lastColumnOfLine: line)
        ]
    ]
].

TextEditorCursor ![
moveRight
    | columnCounts | 
    column := column + 1.
    columnCounts := textBuffer lastColumnOfLine: line.
    column > columnCounts ifTrue: [
        line < textBuffer lines size ifTrue: [
            line := line + 1.
            column := column + 1.
        ] ifFalse: [
            column := columnCounts.
        ]
    ]
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
lastColumnOfLine: lineIndex
    lineIndex <= lines size ifTrue: [
        ^ (lines at: lineIndex) size + 1
    ].

    ^ 1
].


TextEditorBuffer ![
insertText: text atCursorPosition: cursor
   | line leftText rightText cursorColumn insertedLine |
    line := lines at: cursor line.

    cursorColumn := cursor column - 1.
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
    cursor textBuffer: textBuffer.
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
            cursorPositionX := (fontFace measureTextExtent: line until: cursor column - 1) x.
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
    anEvent isReturn ifTrue: [
        Stdio stdout nextPutAll: 'Return'; lf
    ].
    anEvent isBackspace ifTrue: [
        Stdio stdout nextPutAll: 'Backspace'; lf
    ].
    anEvent isLeft ifTrue: [
        cursor moveLeft.
        self changed
    ].
    anEvent isRight ifTrue: [
        cursor moveRight.
        self changed
    ].
    anEvent isUp ifTrue: [
        cursor moveUp.
        self changed
    ].
    anEvent isDown ifTrue: [
        cursor moveDown.
        self changed
    ].

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
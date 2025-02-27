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

TextEditorCursor ![
moveUp
    line > 1 ifTrue: [
        line := line - 1.
    ] ifFalse: [
        line := 1.
        column := 1
    ]
].

TextEditorCursor ![
moveDown
    line < textBuffer lines size ifTrue: [
        line := line + 1.
    ] ifFalse: [
        column := textBuffer lastColumnOfLine: line
    ]
].

TextEditorCursor ![
moveHome
    column := 1.
].

TextEditorCursor ![
moveEnd
    column := textBuffer lastColumnOfLine: line
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
splitLineAtCursor: cursor
    | line cursorLine cursorColumn leftText rightText |
    cursorLine := cursor line.
    line := lines at: cursorLine.

    cursorColumn := cursor column - 1.
    leftText := line first: cursorColumn.
    rightText := line allButFirst: cursorColumn.

    lines at: cursorLine put: leftText.
    lines add: rightText after: cursorLine.

    cursor line: cursorLine + 1.
    cursor column: 1.
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

TextEditorBuffer ![
backspaceDeleteAtCursor: cursor
    | cursorColumn cursorLine line leftText rightText previousLine nextLine joinedLine |
    cursorColumn := cursor column - 1.
    cursorLine := cursor line.
    cursorColumn >= 1 ifTrue: [
        line := lines at: cursorLine.
        leftText := line first: cursorColumn - 1.
        rightText := line allButFirst: cursorColumn.

        lines at: cursorLine put: leftText , rightText.
        cursor column: leftText size + 1
    ] ifFalse: [
        cursorLine > 1 ifTrue: [
            previousLine := lines at: cursorLine - 1.
            nextLine := lines at: cursorLine.
            joinedLine := previousLine , nextLine.

            lines at: cursorLine - 1 put: joinedLine.
            lines removeAt: cursorLine.

            cursor line: cursorLine - 1.
            cursor column: previousLine size + 1.
        ]
    ]
].

TextEditorBuffer ![
forwardDeleteAtCursor: cursor

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
        textBuffer splitLineAtCursor: cursor.
        self changed.
    ].
    anEvent isBackspace ifTrue: [
        textBuffer backspaceDeleteAtCursor: cursor.
        self changed
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

    anEvent isHome ifTrue: [
        cursor moveHome.
        self changed
    ].

    anEvent isEnd ifTrue: [
        cursor moveEnd.
        self changed
    ].

    anEvent isDelete ifTrue: [
        textBuffer forwardDeleteAtCursor: cursor.
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
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

Object subclass: #TextEditorSelection instanceVariables: #(startLine startColumn endLine endColumn).

TextEditorSelection ![
initialize
    startLine := 1.
    startColumn := 1.
    endLine := 1.
    endColumn := 1.
].

TextEditorSelection ![
startLine
    ^ startLine
].

TextEditorSelection ![
startLine: aLineIndex
    startLine := aLineIndex 
].

TextEditorSelection ![
startColumn
    ^ startColumn
].

TextEditorSelection ![
startColumn: aColumnIndex
    startColumn := aColumnIndex 
].

TextEditorSelection ![
endLine
    ^ endLine
].

TextEditorSelection ![
endLine: aLineIndex
    endLine := aLineIndex 
].

TextEditorSelection ![
endColumn
    ^ endColumn
].

TextEditorSelection ![
endColumn: aColumnIndex
    endColumn := aColumnIndex 
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
insertText: text replacingSelection: selection
    "TODO: Implement this"
].

TextEditorBuffer ![
insertText: text atEndOfLine: lineIndex
    lines at: lineIndex put: (lines at: lineIndex) , text
].

TextEditorBuffer ![
getTextAsString
    | textSize textString destIndex line |
    textSize := 0.
    1 to: lines size do: [:lineIndex |
        textSize := textSize + (lines at: lineIndex) size
    ].

    textString := String basicNew: textSize.
    destIndex := 1.
    1 to: lines size do: [:lineIndex |
        line := lines at: lineIndex.
        1 to: line basicSize do: [:charIndex |
            textString basicAt: destIndex put: (line basicAt: charIndex).
            destIndex := destIndex + 1
        ].
    ].

    textSize
].

TextEditorBuffer ![
setTextFromString: aString
    | newLines |
    newLines := ArrayList new.
    aString lines do: [:each |
        newLines add: each
    ].

    lines := newLines.
].

TextEditorBuffer ![
getTextAtSelection: selection
    selection == nil ifTrue: [^ self getTextAsString]
    "TODO: Implement this"
].

TextEditorBuffer ![
getTextAtLine: lineIndex
    ^ lines at: lineIndex
].

TextEditorBuffer ![
getTextAtCursorLine: cursor
    ^ lines at: cursor line
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

BorderedMorph subclass: #TextAreaMorph instanceVariables: #(textBuffer textColor selection selectionColor cursor cursorColor).

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
    selection := nil.
    selectionColor := Color blue.
].

TextAreaMorph ![
contents
    ^ textBuffer getTextAsString
].

TextAreaMorph ![
contents: aString
    textBuffer setTextFromString: aString.
    self changed
].

TextAreaMorph ![
fontFace
    ^ Font defaultMonospacedFontFace
].

TextAreaMorph ![
renderSelectionWithBuilder: renderingBuilder
    | fontFace lineSpace lines line minLine minColumn maxLine maxColumn lineY startX endX |
    fontFace := self fontFace.
    lineSpace := fontFace linespace.
    lines := textBuffer lines.

    minLine := selection startLine min: selection endLine.
    maxLine := selection startLine max: selection endLine.
    minColumn := selection startColumn min: selection endColumn.
    maxColumn := selection startColumn max: selection endColumn.

    minLine = maxLine ifTrue: [
        lineY := (minLine - 1) * lineSpace.
        line := lines at: minLine.
        startX := (fontFace measureTextExtent: line until: minColumn - 1) x.
        endX := (fontFace measureTextExtent: line until: maxColumn - 1) x.
        
        renderingBuilder solidRectangle: (startX@lineY corner: endX@(lineY + lineSpace)) color: selectionColor.
        ^ self
    ].

    minLine to: maxLine do: [:lineIndex |
        lineIndex = minLine ifTrue: [
            lineY := (lineIndex - 1) * lineSpace.
            line := lines at: lineIndex.
            startX := (fontFace measureTextExtent: line until: minColumn - 1) x.
            endX := (fontFace measureTextExtent: line) x.
            
            renderingBuilder solidRectangle: (startX@lineY corner: endX@(lineY + lineSpace)) color: selectionColor.
        ] ifFalse: [
            lineIndex = maxLine ifTrue: [
                lineY := (lineIndex - 1) * lineSpace.
                line := lines at: lineIndex.
                startX := 0.
                endX := (fontFace measureTextExtent: line until: maxColumn - 1) x.
                
                renderingBuilder solidRectangle: (startX@lineY corner: endX@(lineY + lineSpace)) color: selectionColor.
            ] ifFalse: [
                lineY := (lineIndex - 1) * lineSpace.
                line := lines at: lineIndex.
                startX := 0.
                endX := (fontFace measureTextExtent: line) x.
                
                renderingBuilder solidRectangle: (startX@lineY corner: endX@(lineY + lineSpace)) color: selectionColor.

            ]
        ]
    ].
].

TextAreaMorph ![
renderWithBuilder: renderingBuilder
    | fontFace lines lineSpace lineWidth linePositionY |
    fontFace := self fontFace.
    lineSpace := fontFace linespace.

    renderingBuilder solidRectangle: self localBounds color: self color borderSize: self borderWidth borderColor: self borderColor.
    
    lines := textBuffer lines.
    linePositionY := 0.

    selection ~~ nil ifTrue: [
        self renderSelectionWithBuilder: renderingBuilder.
    ].

    1 to: lines size do: [:lineIndex |
        | line lineExtent lineRectangle cursorPositionX cursorBounds |
        line := lines at: lineIndex.
        lineExtent := fontFace measureTextExtent: line.
        lineRectangle := 0@linePositionY extent: lineExtent.

        renderingBuilder text: line rectangle: lineRectangle color: textColor fontFace: fontFace.

        lineIndex = cursor line ifTrue: [
            cursorPositionX := (fontFace measureTextExtent: line until: cursor column - 1) x.
            cursorBounds := cursorPositionX @ linePositionY extent: 2@lineSpace.
            renderingBuilder solidRectangle: cursorBounds color: cursorColor.
        ].

        linePositionY := linePositionY + lineSpace.
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
    | hasChanged |
    hasChanged := false.

    anEvent isReturn ifTrue: [
        textBuffer splitLineAtCursor: cursor.
        hasChanged := true.
    ].
    anEvent isBackspace ifTrue: [
        textBuffer backspaceDeleteAtCursor: cursor.
        hasChanged := true.
    ].

    anEvent hasShift ifTrue: [
        selection == nil ifTrue: [
            selection := TextEditorSelection new
                startLine: cursor line;
                startColumn: cursor column;
                endLine: cursor line;
                endColumn: cursor column;
                yourself
        ]
    ] ifFalse: [
        selection := nil
    ].

    anEvent isLeft ifTrue: [
        cursor moveLeft.
        hasChanged := true.
    ].
    anEvent isRight ifTrue: [
        cursor moveRight.
        hasChanged := true.
    ].
    anEvent isUp ifTrue: [
        cursor moveUp.
        hasChanged := true.
    ].
    anEvent isDown ifTrue: [
        cursor moveDown.
        hasChanged := true.
    ].

    anEvent isHome ifTrue: [
        cursor moveHome.
        hasChanged := true.
    ].

    anEvent isEnd ifTrue: [
        cursor moveEnd.
        hasChanged := true.
    ].

    anEvent hasShift ifTrue: [
        selection ~~ nil ifTrue: [
            selection endLine: cursor line;
                endColumn: cursor column;
                yourself
        ]
    ] ifFalse: [
        selection := nil
    ].

    anEvent isDelete ifTrue: [
        textBuffer forwardDeleteAtCursor: cursor.
        hasChanged := true.
    ].

    anEvent wasHandled: true.
    hasChanged ifTrue: [self changed].
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
    "selection ~~ nil ifTrue: [
        textBuffer insertText: anEvent text replacingSelection: selection.
        selection := nil.
    ] ifFalse: ["
        textBuffer insertText: anEvent text atCursorPosition: cursor.
    "]."
    anEvent wasHandled: true.
    self changed.
].
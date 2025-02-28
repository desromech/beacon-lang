TextAreaMorph subclass: #CodeTextAreaMorph instanceVariables: #(language).

Object subclass: #CodeTextAreaLanguage instanceVariables: #(codeAreaMorph).

CodeTextAreaLanguage ![
codeAreaMorph
    ^ codeAreaMorph
].

CodeTextAreaLanguage ![
codeAreaMorph: aCodeAreaMorph
    codeAreaMorph := aCodeAreaMorph
].

CodeTextAreaLanguage ![
printIt: codeText
    Stdio stdout nextPutAll: 'TODO printIt: '; nextPutAll: codeText; lf
].

CodeTextAreaLanguage ![
inspectIt: codeText
    Stdio stdout nextPutAll: 'TODO inspectIt: '; nextPutAll: codeText; lf    
].

CodeTextAreaLanguage ![
doIt: codeText
    Stdio stdout nextPutAll: 'TODO doIt: '; nextPutAll: codeText; lf    
].

CodeTextAreaLanguage ![
debugIt: codeText
    Stdio stdout nextPutAll: 'TODO debugIt: '; nextPutAll: codeText; lf
].

CodeTextAreaLanguage subclass: #CodeTextAreaBeaconLanguage.

CodeTextAreaBeaconLanguage ![
compileAndRunIt: codeText
    ^ SourceCode new
        directory: '.';
        name: 'Workspace';
        text: codeText;
        textSize: codeText size;
        compileAndRunIt
].

CodeTextAreaBeaconLanguage ![
printIt: codeText
    | result |
    result := self compileAndRunIt: codeText.
    codeAreaMorph insertTextAtEndOfCurrentSelection: result printString
].

CodeTextAreaBeaconLanguage ![
inspectIt: codeText
    | result |
    result := self compileAndRunIt: codeText.
    Stdio stdout nextPutAll: result printString; lf
].

CodeTextAreaBeaconLanguage ![
doIt: codeText
    self compileAndRunIt: codeText.
].

CodeTextAreaMorph ![
initialize
    super initialize.
    language := CodeTextAreaBeaconLanguage new
        codeAreaMorph: self;
        yourself.

].

CodeTextAreaMorph ![
handleMorphKeyboardDownEvent: anEvent
    anEvent hasControl ifTrue: [
        anEvent symbol = $p asInteger ifTrue: [
            self printIt
        ].
        anEvent symbol = $d asInteger ifTrue: [
            anEvent hasShift ifTrue: [
                self debugIt
            ] ifFalse: [
                self doIt
            ]
        ].
        anEvent symbol = $i asInteger ifTrue: [
            self inspectIt
        ].
    ].

    super handleMorphKeyboardDownEvent: anEvent
].

CodeTextAreaMorph ![
getSelectedTextCode
    ^ textBuffer lines at: cursor line
].

CodeTextAreaMorph ![
insertTextAtEndOfCurrentSelection: textToInsert
    textBuffer insertText: textToInsert atEndOfLine: cursor line.
    self changed
].


CodeTextAreaMorph ![
printIt
    language printIt: self getSelectedTextCode
].

CodeTextAreaMorph ![
inspectIt
    language inspectIt: self getSelectedTextCode
].

CodeTextAreaMorph ![
doIt
    language doIt: self getSelectedTextCode
].

CodeTextAreaMorph ![
debugIt
    language debugIt: self getSelectedTextCode

].

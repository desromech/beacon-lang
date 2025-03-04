String ![
first: size
    | resultSize result |
    resultSize := self basicSize.
    size <= resultSize ifTrue: [
        resultSize := size
    ].

    result := String basicNew: resultSize.
    1 to: resultSize do: [:i |
        result basicAt: i put: (self basicAt: i)
    ].

    ^ result
].

String ![
allButFirst: skippedCount
    | mySize resultSize result |
    mySize := self basicSize.
    skippedCount >= mySize ifTrue: [^ ''].

    resultSize := mySize - skippedCount.
    result := String basicNew: resultSize.
    1 to: resultSize do: [:i |
        result basicAt: i put: (self basicAt: skippedCount + i)
    ].

    ^ result
].

String ![
lines
    | list line startLineIndex index size cr lf char |
    list := ArrayList new.
    cr := Character cr.
    lf := Character lf.

    startLineIndex := 1.
    index := 1.
    size := self size.
    [index <= size] whileTrue: [
        char := self at: index.
        char = cr ifTrue: [
            ((index + 1 <= size) and: [(self at: index + 1) = lf]) ifTrue: [
                index := index + 1.
                line := self copyFrom: startLineIndex to: index - 1.
                list add: line.
                startLineIndex := index + 1
            ] ifFalse: [
                line := self copyFrom: startLineIndex to: index - 1.
                list add: line.
                startLineIndex := index + 1
            ]
        ] ifFalse: [
            char = lf ifTrue: [
                line := self copyFrom: startLineIndex to: index - 1.
                list add: line.
                startLineIndex := index + 1
            ]
        ].

        index := index + 1
    ].

    startLineIndex <= size ifTrue: [
        line := self copyFrom: startLineIndex to: size.
        list add: line.
    ].

    ^ list asArray
].

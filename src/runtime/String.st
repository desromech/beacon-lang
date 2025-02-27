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

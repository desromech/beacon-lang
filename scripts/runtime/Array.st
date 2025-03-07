Array ![
asArray
    ^ self
].

Array ![
copyWith: extraElement
    | result |
    result := Array basicNew: self basicSize + 1.
    1 to: self basicSize do: [:index |
        result basicAt: index put: (self basicAt: index)
    ].
    result basicAt: self basicSize + 1 put: extraElement.
    ^ result
].

Array ![
copyWithout: elementToRemove
    | resultSize result resultIndex|
    resultSize := 0.
    1 to: self basicSize do: [:index |
        (self basicAt: index) = elementToRemove ifFalse:[
            resultSize := resultSize + 1
        ]
    ].

    result := Array new: resultSize.
    resultIndex := 1
    1 to: self basicSize do: [:index |
        | element |
        element := self basicAt: index.
        element = elementToRemove ifFalse:[
            result basicAt: resultIndex put: element.
            resultIndex := resultIndex + 1
        ]
    ].

    ^ result
].

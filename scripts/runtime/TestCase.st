Behavior ![
selectors
    ^ methodDict == nil
        ifTrue: [ #() ]
        ifFalse: [methodDict keys]
].

TestCase class ![
testSelectors
    
    ^ self selectors select: [:eachSelector | eachSelector beginsWith: 'test'].
].

TestCase class ![
allTestSelectors
    ^ self testSelectors

].

TestCase ![
testDummy
].

TestCase ![
setUp
].

TestCase ![
tearDown
].
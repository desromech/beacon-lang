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

TestCase class ![
run: aSymbol
    ^ (self selector: aSymbol) run
].

TestCase class ![
selector: aSymbol
    ^ self new setTestSelector: aSymbol
].

TestCase ![
setTestSelector: aSymbol
    testSelector := aSymbol
].

TestCase ![
run
    
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


# Learning tests for Cormen, Leiserson, Rivest, Stein: Introduction to Algorithms 3rd ed

Some engineering issues outdated or ignored. This makes it a more difficult reading to
the professional programmer.

Using 1-indexed instead of 0-indexed arrays is just plain outdated. The loop counter value
is available after the loop.

Insersion sort pseudocode re-writes values even if the array is already sorted.

Little attention is given to generic solutions to problems e.g. sorting will need to
be customized by type and comparison function.

End of sequences is treated in a disorganised manner. Sometimes it takes advantage of the
1-indexing to use 0 (Nil) as the "index" for the end of the sequence. In other places a
sentinel value is used for end of the sequence.

Often such shortcuts are taken with the aim to have the code as compact as possible,
disregarding the overall clarity. Sure, the sentinel approach is clever, should be known,
but to the professional a more consistent approach would make a easier reading, you
should not have almost each algorithm use a different approach just to save a few lines
of code.

Another case: the maximum subaray problem is introduced via a buy cheap then sell expensive
problem in the divide and conquer chapter. I read the problem description carefully to make
sure I understand it, then I figure out that I can solve it by linear traversal, then I wonder
why would you use divide and conquer, which over many pages turns out it's O(n*lg(n)), to
finally get to the challange to solve it in O(n) i.e. linear traversal.

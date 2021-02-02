I implemented normal,concurrent and multithread merge sort. concurrent merge sort by creating child processes for each subpart in diving in mersort function. multi thread mergesort created a thread for each subpart of the array in merge sort function. when length of the array is less than 5 i m just using selection sort to sort that part of array.

analysis:
normal merge runs faster than concurrent and multithread merge sort and concurrent mergesort runs faster than multithread merge sort

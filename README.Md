Για την υλοποίηση του προβλήματος χρησιμοποίησα δύο διαφορετικά αρχεία .c, ένα για κάθε διεργασία έτσι όπως περιγράφονται από την εκφώνηση(Α και Β).
Επίσης χρησιμοποιώ ένα header αρχείο(processes.h) το οποίο:
    -> κάνει inclue όλες τις βιβλιοθήκες που χρειάζομαι
    -> υλοποιεί την συνάρτηση init_str η οποία αλλάζει τα περιεχόμενα ενός string σε κενό
    -> υλοποιεί την συνάρτηση cleanup_handler η οποία κάνει free το όρισμα που δέχεται. Αυτη η συνάρτηση είναι απαραίτητη καθώς πρέπει να γίνουν free τα περιεχόμενα των threads και συγκεκριμενα τα string που έχουν γίνει malloc(αυτό δεν μπορεί να γίνει μέσα στις υλοποίησεις των συναρτήσεων των threads καθώς υπάρχει η περίπτωση το thread να γίνει cancel και να μην μπορέσει να κάνει ποτέ το free)
    -> περίεχει την δήλωση του shared memory segment 

H διεργασία Α(process-a.c) κάνει τα εξής:
    -> main: 
        -> Δημιουργούμε το shared memory segment και κάνουμε τις απαραίτητες ενέργειες(ftruncate, mmap)
        -> Αρχικοποιούμε τους σεμαφόρους, οι σεμαφόροι sem_a και sem_c αρχικοποιούνται σε 1 καθώς χρησιμοποιούνται από ένα thread για την είσοδο από τον χρήστη, ενώ οι σεμαφόροι sem_b και sem_d αρχικοποιούνται σε 0 καθώς χρησιμοποιούνται από ένα thread για την παραλαβή ενός μηνύματος από την άλλη διεργασία 
        -> Αρχικοποίηση των μεταβλητών του shared memory segment
        -> Δημιουργία και join των threads 
        -> Εκτύπωση μηνυμάτων 
        -> Αποδέσμευση του shared memory segment 
    -> thread_send_function:
        -> Κατεβάζει τον sem_a και μπαίνει στο critical section
        -> πάντα διαγράφει τα περιέχομενα του buf_a που βρίσκεται στο shared memory segment 
        -> εάν παίρνει καινούριο μήνυμα από τον χρήστη τότε αρχικοποιεί κάποιες μεταβλητές και βρίσκει σε πόσα τεμάχια θα χωριστεί το μήνυμα. Εάν το μήνυμα ξεκινάει από #BYE# και μετά περιέχει την αλλαγή γραμμής(\n) τότε οι διεργασίες θα πρέπει να σταματήσουν(χρησιμοποιείται η αλλαγή γραμμής αντί για \0 καθώς μετά από το τέλος του μηνύματος του χρήστη η fgets προσθέτει πρώτα τον χαρακτήρα \n και μετά τον \0)
        -> αντιγράφει το συγκεκριμένο τεμάχιο του μηνύματος στον buffer του shared memory segment
        -> Ανεβάζει τον sem_b έτσι ώστε να ξεμπλοκάρει το thread_receive_function της διεργασίας Β
    -> thread_receive_function:
        -> Η gettimeofday χρησιμοποείται για να βρούμε πόσα δευτερολέπτα περιμένει το thread μήνυμα από τον χρήστη
        -> Κατεβάζει τον sem_b και μπαίνει στο critical section
        -> Εάν το τεμάχιο που διαβάζει είναι από καινούριο μήνυμα, τότε αρχικοποιέι το string και ελέγχει εάν είναι ίσο με το μήνυμα τερματισμού
        -> Διαφορετικά, προσθέτει το BUFFER_SIZE στο offset του τωρινού string
        -> Αντιγράφει το συγκεκριμένο τεμάχιο από τον buffer στο string 
        -> Εάν είναι το τελευταίο τεμάχιο του μηνύματος τότε εκτυπώνει το μήνυμα
        -> Ανεβάζει τον sem_a έτσι ώστε να μπορεί να ξαναδώσει είσοδο ο χρήστης στην διεργασία Β.

Η διεργασία Β(process-b.c) κάνει τα εξής:
    -> main: Παρόμοια με την main της διεργασίας Α, όμως δεν δημιουργεί το shared memory segment ούτε το αποδεσμεύει, απλά το ανοίγει.
    Επίσης δεν αρχικοποιεί τους σεμαφόρους και τις μεταβλητές του shared memory segment. 
    -> thread_send_function και thread_receive_function: παρόμοιες με τις αντίστοιχες της διεργασίας Α, απλά χρησιμοποιούν διαφορετικούς σεμαφόρους(sem_c και sem_d)


Ενδεικτικό αποτέλεσμα προσομοίωσης:
./process-a
Shared memory object: /process has been created at address: 0x7fa48dc59000
123456
sending:123456

123456789012356
sending:123456789012345
sending:

Process A read:This is a random text

This is a really long text, to see if the chunks are divided correctly
sending:This is a reall
sending:y long text, to
sending: see if the chu
sending:nks are divided
sending: correctly

#BYE#
sending:#BYE#

Number of messages sent:3
Number of messages received:1
Number of chunks sent:8
Number of chunks received:2
Average of chunks sent:2.67
Average of chunks received:2.00
Average time waiting to receive first chunk:68.00


./process-b
Shared memory object: /process has been created at address: 0x7f1f6a6f2000
Process B read:123456

Process B read:123456789012345

This is a random text
sending:This is a rando
sending:m text

Process B read:This is a really long text, to see if the chunks are divided correctly

Process B read:#BYE#

Number of messages sent:1
Number of messages received:3
Number of chunks sent:2
Number of chunks received:8
Average of chunks sent:2.00
Average of chunks received:2.67
Average time waiting to receive first chunk:84.67

Σχολιασμός αποτελεσμάτων:   
Αρχικά η διεργασία Α παίρνει σαν είσοδο από τον χρήστη το μήνυμα "123456" και το στέλνει ως ένα τεμάχιο(αφού χωράει στο shared memory segment buffer) στην διεργασία Β
Στην συνέχεια η διεργασία Α παίρνει σαν είσοδο το μήνυμα "123456789012345" το οποίο έχει 15 χαρακτήρες(όσο και το μέγεθος του buffer) όμως περίεχει και τον χαρακτήρα '\0' ο οποίος πρέπει να περαστεί σε δεύτερο χρόνο καθώς δεν χωράει. Η διεργασία Β αποθηκεύει τα τεμάχια και εκτυπώνει τελικά ολόκληρο το μήνυμα.
Αντίστοιχα μετά η διεργασία Β δέχεται από τον χρήστη το μήνυμα "This is a random text" το οποίο πρέπει να χωριστεί σε 2 τεμάχία κ.ο.κ.
Στο τέλος και οι δυο διεργασίες εκτυπώνουν τα πόσα μηνύματα έλαβαν/έστειλαν, όπως και τα τεμάχια. Επίσης εκτυπώνουν το μέσο χρόνο αναμονής για την παραλαβή του πρώτου τεμαχίου νέου μηνύματος.


Αντίστοιχα ο χρήστης έχει την επιλογή αντί για να εκτυπώθουν τα μηνύματα στον τερματικό, να γραφούν σε ένα αρχείο χρησιμοποιώντας ανακατεύθυνση.
Για παράδειγμα, για τις ίδιες εισόδους από τον χρήστη με την προηγούμενη προσομοίωση και τρέχοντας τις διεργασίες με ./process-a > a-output.txt και ./process-b > b-output.txt, έχουμε τα ίδια αποτελέσματα γραμμένα αυτήν την φορά σε αρχεία(το μόνο που έχει αλλάξει είναι ο χρόνος αναμονής).
d:\papyrus\tools\perl\perl make-srgb.pl > pixman-srgb.c

d:\papyrus\tools\perl\perl make-combine.pl  8 < pixman-combine.c.template > pixman-combine32.c
d:\papyrus\tools\perl\perl make-combine.pl  8 < pixman-combine.h.template > pixman-combine32.h

d:\papyrus\tools\perl\perl make-combine.pl 16 < pixman-combine.c.template > pixman-combine64.c
d:\papyrus\tools\perl\perl make-combine.pl 16 < pixman-combine.h.template > pixman-combine64.h





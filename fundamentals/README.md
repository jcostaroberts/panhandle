A simple program for getting basic metrics on a company. Typically as I look through a financial statement I'll add relevant fields as arguments to the program. No field is required--it just does what it can with the fields provided.

It looks like this:

```
% ./fundamentals -n USCR -P 985.19m -p 63.5 -q 4 -e 0.59 -e -0.38\
> -a 945402t -g 133271t -i 130973t -l 756573t

Company                  USCR
Market cap               $985.19m
P/E                      604.762
P/B                      -13.064
D/E                      -10.032

% ./fundamentals -n USCR -P 985.19m -p 63.5 -d 0 -d 0 -q 4 -e 0.59\
> -e -0.38 -a 945402t -g 133271t -i 130973t -l 756573t -A 341289t\
> -L 270006t -I 41979t -t 16654t -T 432664t -r 54852t -R 0 -o 58438t\
> -c 75774t -C 0 -m 0

Company                  USCR
Market cap               $985.19m
EV                       $1.36b
EBITDA                   $113.29m
EV/EBITDA                11.993
P/E                      604.762
P/B                      -13.064
Dividend yield           0.000
Dividend coverage        inf
Current ratio            1.264
Quick ratio              1.109
D/E                      -10.032
STD/E                    0.024
LTD/E                    0.635
STD/D                    0.037
LTD/D                    0.963
```

#!/usr/local/bin/perl -w

while(<STDIN>) {

($key,$val) = split('=',$_,2);

if($key eq "LOC_NAME") { print "LOC_NAME=Piglatin-EN\n"; }
elsif($key eq "LOC_DESC") { print "LOC_DESC=Piglatin made from English\n"; }
elsif(/.+\=.+/) {
	$val=~s/
         \b                            # start of word
         (                             # capture all to $1
           (                           # this is $2
             qu                        # word starts with qu
             |                         # or
             [bcdfghjklmnpqrstvwxyz]+  # a consonent
           )?                          # but it's optional
           (                           # this is $3
              [a-z]+                   # rest of word
           )
         )
        /$2 ? $3.$2."ay" : $1."way" /ixeg;   # if $2 evaluates as true then 
                                            # put it at end of word and add "ay"
                                            # otherwise, just add "way"
	print "$key=$val";
} else { print $_; }

}

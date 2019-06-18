# JSquash
My fun JS minifier

I used to use Google Closure Compiler but I fancied having a crack at it myself, because I'm a control freak and want to know exactly what's going on.

So I knocked this thing up (VS 2017) as a fun exercise. It's sufficient for my needs, but it's hardly optimal and probably won't please the purists. It's just an console application that expects a single Javascript file in, and the name of an output file to write your minified Javvascript to; plus a couple of options, eg.

      JSquash.exe fred.js fred_min.js -s -rcw
      
which means work on fred.js and spew out fred_min.js and substitute the symbols for short names (-s) and remove comments and whitespace (-rcw).

-- Prog contains normal ops + return + node a b = do a then do b
data Prog = Dup | Drop | Swap | Cat | Quote | Run | Push Prog | Node Prog Prog | Ret
-- data stack -> return stack -> current op -> stack after program is done
go :: [Prog] -> [Prog] -> Prog -> [Prog] -- evaluate/reduce the program
go st [] Ret = st
go st (r:rs) Ret = go st rs r
go st rs (Node a Ret) = go st rs a -- tail call
go st rs (Node a b) = go st (b:rs) a
-- non ctrl flow ops defer to return to check if the prog is done running
go st rs (Push a) = go (a:st) rs Ret
go (s:st) rs Run = go st rs s
go (s:st) rs Quote = go (Push s:st) rs Ret
go (b:a:st) rs Cat = go (Node a b:st) rs Ret
go (b:a:st) rs Swap = go (a:b:st) rs Ret
go (_:st) rs Drop = go st rs Ret
go (s:st) rs Dup = go (s:s:st) rs Ret
str :: Prog -> [Char] -- convert a program to a string
str Ret = ""
str (Node a b) = str a ++ str b
str (Push a) = "(" ++ str a ++ ")"
str Run = "^"
str Quote = "a"
str Cat = "*"
str Swap = "~"
str Drop = "!"
str Dup = ":"
parse :: [Char] -> (Prog,[Char]) -- parse a string into a prog and the unparsed leftovers
parse ('(':rest) = (Node (Push inner) after,rest'') where
	(inner,rest') = parse rest -- parse the inside of ()
	(after,rest'') = parse rest' -- parse after
parse (')':rest) = (Ret,rest)
parse "" = (Ret,"")
parse (':':rest) = (Node Dup inner,rest') where (inner,rest') = parse rest
parse ('!':rest) = (Node Drop inner,rest') where (inner,rest') = parse rest
parse ('~':rest) = (Node Swap inner,rest') where (inner,rest') = parse rest
parse ('*':rest) = (Node Cat inner,rest') where (inner,rest') = parse rest
parse ('a':rest) = (Node Quote inner,rest') where (inner,rest') = parse rest
parse ('^':rest) = (Node Run inner,rest') where (inner,rest') = parse rest
parse (_:rest) = parse rest -- ignore other chars
run :: [Char] -> [Char] -- 'reduce' ul program to one without any top level ops
-- read pipeline right to left
--    convert finished stack to string                   run the prog  parse
run = concat . reverse . map (("("++) . (++")") . str) . go [] [] .    fst . parse
main = interact run
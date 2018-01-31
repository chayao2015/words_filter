local filter,err = require("words_filter").WordsFilter(
	{
		"毛泽东",
		"fuck",
		"习主席"
	}
)

if nil == err then
	print(filter:Filtrate("你妈妈fuck得的","*"))
	print(filter:Filtrate("我不喜欢毛泽东","*"))
	print(filter:Filtrate("你妈妈得的","*"))
	print(filter:Filtrate("习主席很开心","*"))	
	print(filter:Check("习主席很开心"))
	print(filter:Check("hello world"))			
end
import jsonpath24

# t = {
#     "name": "basic, wildcard shorthand, object data",
#     "selector": "$.*",
#     "document": {"a": "A", "b": "B"},
#     "result": ["A", "B"],
# }

t = {
    "name": "functions, value, single-value nodelist",
    "selector": "$[?value(@.*)==4]",
    "document": [[4], {"foo": 4}, [5], {"foo": 5}, 4],
    "result": [[4], {"foo": 4}],
}


print("COMPILE: ", t["selector"], "...")
p = jsonpath24.compile(t["selector"])
print("COMPILED TO: ", repr(p))

print("QUERYING DATA ...")
rv = p.findall(t["document"])

if rv == t["result"]:
    print("PASS!!")
else:
    print("FAIL!!")

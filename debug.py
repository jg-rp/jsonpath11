import jsonpath24

t = {
    "name": "basic, wildcard shorthand, object data",
    "selector": "$.*",
    "document": {"a": "A", "b": "B"},
    "result": ["A", "B"],
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

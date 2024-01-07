# jsonpath24

JSONPath for Python, built on [libjsonpath](https://github.com/jg-rp/libjsonpath), following the draft [IETF JSONPath Base](https://datatracker.ietf.org/doc/html/draft-ietf-jsonpath-base-21) specification.

## jsonpath24 vs python-jsonpath

jsonpath24 is faster than [python-jsonpath](https://github.com/jg-rp/python-jsonpath), but less featureful. If you're not concerned wih JSONPath query performance, you should probably use python-jsonpath.

Both jsonpath24 and python-jsonpath follow the IETF JSONPath base specification and are tested against the [JSONPath compliance test suite](https://github.com/jsonpath-standard/jsonpath-compliance-test-suite). Additionally, python-jsonpath:

- works on data made up of any [mapping](https://docs.python.org/3/glossary.html#term-mapping) or [sequence](https://docs.python.org/3/glossary.html#term-sequence), not just dictionaries and lists.
- supports membership operators (`in` and `contains`) and list/array literals. <sup>\*</sup>
- supports references to keys/properties within filter selectors. <sup>\*</sup>
- supports extra filter context data. <sup>\*</sup>
- supports selecting keys/properties with the `~` selector. <sup>\*</sup>
- supports custom async mappings via `__getitem_async__` for lazy loading data.
- includes a command line interface.
- includes JSON Pointer and JSON Patch implementations.
- allows for some JSONPath parser customization by subclassing `Lexer` and `Parser`.
- allows for some JSONPath resolution customization by subclassing `JSONPathEnvironment`.

<sup>\*</sup> Not part of the JSONPath base standard.

### Performance

Repeating 369 queries from the [compliance test suite](https://github.com/jsonpath-standard/jsonpath-compliance-test-suite) 100 times. These are very small queries on very small data.

All times are taken from a best of three run using Python 3.9 on the same machine.

| Impl                                                        | Compile and find (values) | Compile and find (nodes) | Just compile | Just find (values) | Just find (nodes) |
| ----------------------------------------------------------- | ------------------------- | ------------------------ | ------------ | ------------------ | ----------------- |
| [python-jsonpath](https://github.com/jg-rp/python-jsonpath) | 3.78 s                    | 3.76 s                   | 2.45 s       | 0.92 s             | 1.01 s            |
| [jsonpath24](https://github.com/jg-rp/jsonpath24)           | 0.45 s                    | 0.31 s                   | 0.09 s       | 0.40 s             | 0.27 s            |

When querying large datasets and producing a large number of results, the difference in performance between jsonpath24 and python-jsonpath is expected to be less significant. This is due to jsonpath24 using `nb::dict` and `nb::list` wrappers for Python dictionaries and lists internally, so we are still limited by Python dict and list performance.

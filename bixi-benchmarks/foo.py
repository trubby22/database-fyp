def foo():
    print("hello")

bar = {
    "baz": foo
}

bar["baz"]()

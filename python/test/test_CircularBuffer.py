from hmtl.CircularBuffer import CircularBuffer


def test_basic_circular():
    buff = CircularBuffer(10)
    buff.put('test')

    assert len(buff) == 1
    assert buff.get() == 'test'


def test_rotation():
    limit = 10
    buff = CircularBuffer(limit)

    for x in range(0, 11):
        buff.put(x)

    assert len(buff) == limit
    assert buff.get() == 1


def test_iteration():
    limit = 10
    buff = CircularBuffer(limit)

    for x in range(0, 10):
        buff.put(x)

    y = 0;
    for x in buff:
        assert x == y
        y += 1


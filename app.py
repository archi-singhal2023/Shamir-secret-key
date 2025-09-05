import json

def get_combinations(iterable, r):
    pool = tuple(iterable)
    n = len(pool)
    if r > n:
        return
    indices = list(range(r))
    yield tuple(pool[i] for i in indices)
    while True:
        for i in reversed(range(r)):
            if indices[i] != i + n - r:
                break
        else:
            return
        indices[i] += 1
        for j in range(i + 1, r):
            indices[j] = indices[j - 1] + 1
        yield tuple(pool[i] for i in indices)
        
def mod_inverse(a, m):
    m0, x0, x1 = m, 0, 1
    while a> 1:
        q = a // m
        m, a = a % m, m
        x0, x1 = x1 - q * x0, x0
    return x1 + m0 if x1 < 0 else x1

def lagrange_interpolate_at_zer(points, prime):
    secret = 0
    k = len(points)
    for i in range(k):
        xj, yj = points[i]
        numerator, denominator = 1, 1
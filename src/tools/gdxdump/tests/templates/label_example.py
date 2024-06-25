from common import gt, pd, os, gams_system_directory, gdx_files_directory, get_random_ascii_letters

# TODO: Set it to 255 at some point, but so far the tests fail when you do that
ascii_letter_length = 253

m = gt.Container(system_directory=gams_system_directory)

i = gt.Set(m, 'i', records=[
    ('seattle', get_random_ascii_letters(ascii_letter_length)),
    ('san-diego', get_random_ascii_letters(ascii_letter_length))
], description=get_random_ascii_letters(ascii_letter_length))

j = gt.Set(m, 'j', records=[
    ('new-york', get_random_ascii_letters(ascii_letter_length)),
    ('chicago', get_random_ascii_letters(ascii_letter_length)),
    ('topeka', get_random_ascii_letters(ascii_letter_length))
], description=get_random_ascii_letters(ascii_letter_length))

d = gt.Parameter(m, 'd', [i, j], description=get_random_ascii_letters(ascii_letter_length))

dist = pd.DataFrame(
    [
        ('seattle', 'new-york', 2.5),
        ('seattle', 'chicago', 1.7),
        ('seattle', 'topeka', 1.8),
        ('san-diego', 'new-york', 2.5),
        ('san-diego', 'chicago', 1.8),
        ('san-diego', 'topeka', 1.4),
    ],
    columns=[
        get_random_ascii_letters(ascii_letter_length),
        get_random_ascii_letters(ascii_letter_length),
        get_random_ascii_letters(ascii_letter_length)
    ],
)

d.setRecords(dist)

m.write(os.path.join(gdx_files_directory, 'label_example.gdx'))

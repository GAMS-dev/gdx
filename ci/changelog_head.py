import yaml
import sys

with open('changelog.yaml') as fp:
    obj = yaml.load(fp, Loader=yaml.FullLoader)

version, changes = list(obj[0].items())[0]
if sys.argv[1] == 'tag_name':
    print(version)
elif sys.argv[1] == 'description':
    print('\n'.join('- ' + change for change in changes))

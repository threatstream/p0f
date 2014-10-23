import json, sys
x = {'SECRET': 'HPFEEDS_SECRET','PUB_CHANNEL': 'HPFEEDS_CHANNEL','HOST': 'HPFEEDS_HOST','IDENT': 'HPFEEDS_IDENT','PORT': 'HPFEEDS_PORT'}
r = json.load(file("p0f_hpfeeds.json"))
first = True
sys.stdout.write("environment=")
for name in r:
    if not first:
        sys.stdout.write(",")
    first = False
    sys.stdout.write('{}="{}"'.format(x[name], r[name]))
sys.stdout.write("\n")

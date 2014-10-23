# Hpfeeds Collection

Some tools that enable collecting p0f JSON logs using hpfeeds.

## Deployment

```
apt-get install python-dev
cd hpfeeds
virtualenv env
. env/bin/activate
pip install -e git+https://github.com/threatstream/hpfeeds.git#egg=hpfeeds-dev
pip install ujson
pip install cachetools

```

## Configuration

```
HPF_HOST='127.0.0.1'
HPF_PORT='10000'
HPF_IDENT='p0f'
HPF_SECRET='p0fsecret'

cat > p0f_hpfeeds.json <<EOF
{
	"HOST":   "$HPF_HOST",
	"PORT":   $HPF_PORT,
	"IDENT":  "$HPF_IDENT",
	"SECRET": "$HPF_SECRET",
    "PUB_CHANNEL": "p0f.events"
}
EOF
```

## Running

```
./hpfeeds_collector.sh

```

See MHN for more details.

#!/bin/bash

if [ $# -ne 1 ]; then
  echo usage: release.sh version
  exit 1
fi

VERSION=$1

ID=`curl -v https://api.github.com/repos/clowwindy/ShadowVPN/releases?access_token=$GITHUB_TOKEN | python -c "import json,sys
for item in json.load(sys.stdin):
  if item['tag_name']=='$VERSION':
    print item['id']
    break"`
echo $ID

pushd dist

for file in `ls *$VERSION*`; do 
  curl -v -H "Content-Type: application/x-zip" \
    -H "Authorization: token $GITHUB_TOKEN" \
    --data-binary "@$file" \
    https://uploads.github.com/repos/clowwindy/ShadowVPN/releases/$ID/assets?name=$file
done

popd

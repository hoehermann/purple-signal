#!/bin/bash

# makes a .classpath file for use in eclipse (this directory is the project root)

if [[ -z "$@" ]]
then
  echo "Usage: $0 /wherever/signal-cli/lib/* > .classpath"
  exit 1
fi

cat <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<classpath>
	<classpathentry kind="src" path="submodules/signal-cli/src/main/java"/>
	<classpathentry kind="src" path="submodules/signal-cli/lib/src/main/java"/>
	<classpathentry kind="con" path="org.eclipse.jdt.launching.JRE_CONTAINER">
		<attributes>
			<attribute name="module" value="true"/>
		</attributes>
	</classpathentry>
	<classpathentry kind="output" path="bin"/>
EOF
for i in $@
do
  echo '	<classpathentry kind="lib" path="'$i'"/>'
done
echo '</classpath>'

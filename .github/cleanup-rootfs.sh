#!/bin/bash

# There is no need to wait for this to finish, at least the rm part can be done
# while the process moves forward (for apt we'll need to wait for it to finish
# before we install dependencies later on, but it'll only give us a 1-3GBs so
# we can skip it.
WAIT=0
RM=1
APT=0
DOCKER=1

PACKAGES=(
	"firefox"
	"google-chrome-stable"
	"microsoft-edge-stable"
	"php-pear"
	"ruby-full"
	"^aspnetcore-.*"
	"^dotnet-.*"
	"powershell*"
)

PATHS=(
	"/opt/hostedtoolcache"
	"/usr/local/.ghcup/"
	"/usr/share/swift"
	"/usr/local/lib/android"
	"/usr/local/share/edge_driver"
	"/usr/local/share/gecko_driver"
	"/usr/local/share/chromedriver-linux64"
	"/usr/local/share/chromium"
	"/home/linuxbrew"
	"/usr/local/share/vcpkg"
	"/usr/share/kotlinc"
	"/usr/local/bin/minikube"
)


function cleanup_packages()
{
	if [[ ${APT} == 1 ]]; then
		apt-get purge -y "${PACKAGES[@]}"
		apt-get autoremove --purge -y
		apt-get clean
	fi
}

function cleanup_paths()
{
	if [[ ${RM} == 1 ]]; then
		for i in "${PATHS[@]}"; do
			rm -rf "${i}" &
		done
		if [[ ${WAIT} == 1 ]]; then
			wait
		fi
	fi
}

function cleanup_docker()
{
	if [[ ${DOCKER} == 1 ]]; then
		docker image prune --all --force &
		if [[ ${WAIT} == 1 ]]; then
			wait
		fi
	fi
}

if [[ ${WAIT} == 1 ]]; then
	echo "---=== Before ===---"
	df -hT
	cleanup_packages
	cleanup_paths
	cleanup_docker
	echo "---=== After ===---"
	df -hT
else
	cleanup_packages
	cleanup_paths
	cleanup_docker
fi

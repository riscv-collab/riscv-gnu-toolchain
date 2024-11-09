#!/bin/bash

function deduplicate_files() {
	local DUPLICATES=()
	local DIR=${1}
	local IFS=$'\n'
	local LINK_CHECK=""

	readarray -t DUPLICATES < <(for i in `find ${DIR} -type f ! -empty`; do sha1sum ${i}; done | sort | uniq -w 40 --all-repeated=separate)

	for ((i=1; i < ${#DUPLICATES[@]}; i++ )); do
		if [[ ${DUPLICATES[$i]} == "" ]]; then
			continue
		elif [[ ${DUPLICATES[$i-1]} = "" ]]; then
			continue
		else
			LINK_CHECK=$(ls -li "${DUPLICATES[$i]:42}" "${DUPLICATES[$i-1]:42}" |awk '{print $1}' | uniq | wc -l)
			if [[ ${LINK_CHECK} != "1" ]]; then
				ln -f "${DUPLICATES[$i-1]:42}" "${DUPLICATES[$i]:42}"
			fi
		fi
	done

	return 0
}

deduplicate_files ${1}

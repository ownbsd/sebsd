divert(-1)
#
# Copyright (c) 2000 Sendmail, Inc. and its suppliers.
#	All rights reserved.
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the sendmail distribution.
#

divert(0)
VERSIONID(`$Id$')
divert(-1)

ifdef(`_MAILER_local_',
	`errprint(`*** MAILER(`local') must appear after FEATURE(`local_no_masquerade')')
')dnl
define(`_LOCAL_NO_MASQUERADE_', `1')

/*   
	Custom IOS Module (EHCI)

	Copyright (C) 2008 neimod.
	Copyright (C) 2009 kwiirk.
	Copyright (C) 2009 Hermes.
	Copyright (C) 2009 Waninkoko.
	Copyright (C) 2011 rodries.
	Copyright (C) 2011 davebaol.
	Copyright (C) 2020 Leseratte.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "loop.h"
#include "mem.h"
#include "syscalls.h"
#include "timer.h"
#include "tinyehci.h"
#include "types.h"


char *moduleName = "EHCI";

int main(void)
{
	/* Heap space */
	static u32 heapspace[0x5000] ATTRIBUTE_ALIGN(32);

	s32 ret;

	/* Print info */
	svc_write("$IOSVersion: EHCI:  " __DATE__ " " __TIME__ " 64M " __D2XL_VER__ " $\n");

	/* Initialize memory heap */
	ret = Mem_Init(heapspace, sizeof(heapspace));
	if (ret < 0)
		return ret;

	/* Initialize timer subsystem */
	ret = Timer_Init();
	if (ret < 0)
		return ret;

	/* Set current USB port (Now means LUN instead of USB port 1) */
	current_port = 0;
	
	/* Initialize TinyEhci */
	ret = EHCI_Init();
	if (ret < 0)
		return ret;

	/* Main loop */
	EHCI_Loop();

	return 0;
}

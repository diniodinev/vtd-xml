/* 
* Copyright (C) 2002-2006 XimpleWare, info@ximpleware.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the houne that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "xpath1.h"
static Boolean isUnique_une(unionExpr *une,int i);

Boolean isUnique_une(unionExpr *une, int i){
	return isUniqueIntHash(une->ih,i);
}

unionExpr *createUnionExpr(expr *e){
	exception ee;
	unionExpr *une = (unionExpr *)malloc(sizeof(unionExpr));
	if (une==NULL){
		ee.et = out_of_mem;
		ee.msg = "unionExpr allocation failed ";
		Throw ee;
	}


	une->freeExpr = &freeUnionExpr;
	une->evalBoolean = &evalBoolean_une;
	une->evalNodeSet = &evalNodeSet_une;
	une->evalNumber  = &evalNumber_une;
	une->evalString  = &evalString_une;
	une->isNumerical = &isNumerical_une;
	une->isBoolean = &isBoolean_une;
	une->isString =  &isString_une;
	une->isNodeSet = &isNodeSet_une;
	une->requireContextSize = &requireContextSize_une;
	une->setContextSize = &setContextSize_une;
	une->setPosition = &setPosition_une;
	une->reset = &reset_une;
	une->toString = &toString_une;
	une->fe = e;
	une->next = NULL;
	une->current = une;
	une->evalState = 0;
	une->ih = NULL;
	
	return une;

}
void freeUnionExpr(unionExpr *e){
	unionExpr *tmp, *tmp2;
	e->fe->freeExpr(e->fe);
	freeIntHash(e->ih);
	tmp = e->next;
    free(e);
	while(tmp!= NULL){
		tmp->fe->freeExpr(tmp->fe);
		freeIntHash(tmp->ih);
		tmp2 = tmp;
		tmp = tmp->next;		
		free(tmp2);
	}
}

int	evalNodeSet_une (unionExpr *e,VTDNav *vn){
	exception ee;
	int a;
	if (e->next == NULL) {
		return e->fe->evalNodeSet(e->fe,vn);
	} else {
		while (TRUE) {

			switch (e->evalState) {
				case 0:
					if (e->ih == NULL ){
						Try{
							e->ih = createIntHash();
						}
						Catch(ee){
							Throw ee;
						}
					}
					if (e->current != NULL) {
						push2(vn);
						while ((a = e->current->fe->evalNodeSet(e->current->fe,vn)) != -1) {
							if (isUnique_une(e,a)) {
								e->evalState = 1;
								return a;
							}
						}
						e->evalState = 2;
						pop2(vn);
						break;
					} else
						e->evalState = 3;
					break;

				case 1:
					while ((a = e->current->fe->evalNodeSet(e->current->fe, vn)) != -1) {
						if (isUnique_une(e,a)) {
							e->evalState = 1;
							return a;
						}
					}
					e->evalState = 2;
					pop2(vn);
					break;

				case 2:
					e->current = e->current->next;
					if (e->current != NULL) {
						push2(vn);
						while ((a = e->current->fe->evalNodeSet(e->current->fe, vn)) != -1) {
							if (isUnique_une(e,a)) {
								e->evalState = 1;
								return a;
							}
						}
						pop2(vn);
						break;
					} else
						e->evalState = 3;
					break;

				case 3:
					return -1;

				default:
					ee.et = other;
					ee.msg = "Invalid state evaluating unionExpr";
					Throw ee;
			}
		}
	}
}

double	evalNumber_une (unionExpr *e,VTDNav *vn){
	double d = 0.0;
	exception ee;
	int a = -1;
	int size;
	push2(vn);
	size = vn->contextBuf2->size;
	Try {
		a =evalNodeSet_une(e,vn);
		if (a!=-1){
			if (getTokenType(vn,a)== TOKEN_ATTR_NAME){
				a ++;
			}else if (getTokenType(vn,a)== TOKEN_STARTING_TAG) {
				a = getText(vn);
			}
		}			  
	} Catch (ee){
	}
	vn->contextBuf2->size = size;
	reset_une(e,vn);
	pop2(vn);
	Try{
		if (a!=-1) return parseDouble(vn,a);
	}Catch (ee){
	}
	return 0/d;
}
UCSChar* evalString_une  (unionExpr *e,VTDNav *vn){
	exception ee;
	int size;
	int a = -1;
	push2(vn);
	size = vn->contextBuf2->size;
	
	Try {
		a = evalNodeSet_une(e,vn);
		if (a != -1) {
			if (getTokenType(vn,a) == TOKEN_ATTR_NAME) {
				a++;
			}
			if (getTokenType(vn,a) == TOKEN_STARTING_TAG) {
				a = getText(vn);
			}
		}

	} Catch (ee) {
	}
	vn->contextBuf2->size = size;
	reset_une(e,vn);
	pop2(vn);
	Try {
		if (a != -1)
			return toString(vn,a);
	} Catch (ee) {
		if(ee.et == out_of_mem)
			Throw ee;
	}
	return NULL;	
}
Boolean evalBoolean_une (unionExpr *e,VTDNav *vn){
	exception ee;
	Boolean b = FALSE;
	int size;
	push2(vn);
	// record teh stack size
	size = vn->contextBuf2->size;
    Try{	
		b = (evalNodeSet_une(e,vn) != -1);
	}Catch (ee){
	}
		//rewind stack
	vn->contextBuf2->size = size;
	reset_une(e,vn);
	pop2(vn);
	return b;
}
Boolean isBoolean_une (unionExpr *e){
	return e->fe->isBoolean(e->fe);
}
Boolean isNumerical_une (unionExpr *e){
	return e->fe->isNumerical(e->fe);
}
Boolean isString_une (unionExpr *e){
	return e->fe->isString(e->fe);
}
Boolean isNodeSet_une (unionExpr *e){
	return e->fe->isNodeSet(e->fe);
}
Boolean requireContextSize_une(unionExpr *e){
	return FALSE;
}
void	reset_une(unionExpr *e, VTDNav *vn){
	unionExpr *tmp;
	e->fe->reset(e->fe,vn);
	e->current = e;
	tmp = e->next;
    while (tmp != NULL) {
        tmp->fe->reset(tmp->fe,vn);
        tmp = tmp->next;
    }
	resetIntHash(e->ih);
	e->evalState = 0;


}
void	setContextSize_une(unionExpr *e,int s){
	unionExpr *tmp;
    e->current = e;
    e->current->fe->setContextSize(e->current->fe, s);
    tmp = e->next;
    while (tmp != NULL) {
        tmp->fe->setContextSize(tmp->fe,s);
        tmp = tmp->next;
    }
}

void	setPosition_une(unionExpr *e,int pos){
	unionExpr *tmp;
    e->current = e;
    e->current->fe->setPosition(e->current->fe, pos);
    tmp = e->next;
    while (tmp != NULL) {
        tmp->fe->setPosition(tmp->fe,pos);
        tmp = tmp->next;
    }
}

void    toString_une(unionExpr *e, UCSChar* string){
       if (e->next == NULL)
            e->fe->toString(e->fe,string);
	   else{
            e->fe->toString(e->fe,string);
			wprintf(L" | ");
			e->next->toString(e->next, string);
	   }
}
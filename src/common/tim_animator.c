#include "tim_animator.h"
#include "tim_interpreter.h"
#include <stdio.h>
#include <string.h>

void TIMAnimatorInit(TIMAnimator *animator) {
  memset(animator, 0, sizeof(TIMAnimator));
  TIMInterpreterInit(&animator->_interpreter);
}
void TIMAnimatorRelease(TIMAnimator *animator) {
  TIMInterpreterRelease(&animator->_interpreter);
}

int TIMAnimatorRunAnim(TIMAnimator *animator, TIMHandle *tim) {
  printf("TIMAnimatorRunAnim\n");
  animator->tim = tim;
  return TIMInterpreterExec(&animator->_interpreter, animator->tim);
}

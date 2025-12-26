native func AimSplit_OnAction();

@wrapMethod(PlayerPuppet)
protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
  if action.IsAction(n"FreeLookToggle") {
    LogChannel(n"DEBUG", "AimSplit: FreeLookToggle triggered (Backspace)");
    AimSplit_OnAction();
  }

  // Do not alter handling; let the original method decide.
  return wrappedMethod(action, consumer);
}

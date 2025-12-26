native func AimSplit_OnAction();

@wrapMethod(PlayerPuppet)
protected cb func OnAction(action: ListenerAction, consumer: ListenerActionConsumer) -> Bool {
  let handled = false;

  LogChannel(n"DEBUG", "AimSplit: OnAction wrapper fired");

  if action.IsAction(n"FreeLookToggle") {
    LogChannel(n"DEBUG", "AimSplit: FreeLookToggle triggered (Backspace)");
    AimSplit_OnAction();
    handled = true;
  }

  return wrappedMethod(action, consumer) || handled;
}
